/************************************************************************************

	AstroMenace
	Hardcore 3D space scroll-shooter with spaceship upgrade possibilities.
	Copyright (c) 2006-2018 Mikhail Kurinnoi, Viewizard


	AstroMenace is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	AstroMenace is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with AstroMenace. If not, see <https://www.gnu.org/licenses/>.


	Website: https://www.viewizard.com/
	Project: https://github.com/viewizard/astromenace
	E-mail: viewizard@viewizard.com

*************************************************************************************/

// TODO codestyle should be fixed

// TODO translate comments

#include "../object3d.h"
#include "../space_ship/space_ship.h"
#include "../ground_object/ground_object.h"
#include "../projectile/projectile.h"
#include "../space_object/space_object.h"

// NOTE switch to nested namespace definition (namespace A::B::C { ... }) (since C++17)
namespace viewizard {
namespace astromenace {

namespace {

constexpr float RadToDeg = 180.0f / 3.14159f; // convert radian to degree

} // unnamed namespace

// FIXME should be fixed, don't allow global scope interaction for local variables
extern sVECTOR3D GameCameraMovement;
float GameCameraGetSpeed();


//-----------------------------------------------------------------------------
// Получение угла поворота оружия на врага для космических кораблей
//-----------------------------------------------------------------------------
void GetShipOnTargetOrientation(eObjectStatus ObjectStatus, // статус объекта, который целится
				const sVECTOR3D &Location, // положение точки относительно которой будем наводить
				const sVECTOR3D &CurrentObjectRotation, // текущие углы объекта
				float MinDistance, // минимальное расстояние, с которого начинаем прицеливание
				const float (&RotationMatrix)[9], // матрица вращения объекта
				sVECTOR3D &NeedAngle,// нужные углы, чтобы получить нужное направление
				bool NeedCenterOrientation, // нужен доворот на центр
				bool NeedByWeaponOrientation, // нужно делать доворот с учетом положения орудия
				const sVECTOR3D &WeponLocation,
				int WeaponType) // тип орудия орудия
{
	float tmpDistanceToLockedTarget2{1000.0f * 1000.0f};
	bool TargetLocked{false};

	sVECTOR3D Orientation{0.0f, 0.0f, 1.0f};
	vw_Matrix33CalcPoint(Orientation, RotationMatrix);
	sVECTOR3D PointUp{0.0f, 1.0f, 0.0f};
	vw_Matrix33CalcPoint(PointUp, RotationMatrix);
	sVECTOR3D PointRight{1.0f, 0.0f, 0.0f};
	vw_Matrix33CalcPoint(PointRight, RotationMatrix);

	// vertical plane (left/right), note, OpenGL use right-handed coordinate system
	float A, B, C, D;
	vw_GetPlaneABCD(A, B, C, D, Location, Location + Orientation, Location + PointUp);
	float A2B2C2D2NormalLength = vw_sqrtf(A * A + B * B + C * C);

	// vertical plane (ahead/behind), note, OpenGL use right-handed coordinate system
	float A2, B2, C2, D2;
	vw_GetPlaneABCD(A2, B2, C2, D2, Location, Location + PointRight, Location + PointUp);

	// vertical plane (ahead/behind), note, OpenGL use right-handed coordinate system
	float A3, B3, C3, D3;
	vw_GetPlaneABCD(A3, B3, C3, D3, Location, Location + Orientation, Location + PointRight);
	float A3B3C3D3NormalLength = vw_sqrtf(A3 * A3 + B3 * B3 + C3 * C3);

	// the idea of tmpDistanceFactorByObjectType is provide targeting priority, we increase factor
	// for different types of objects, so, we need next type objects position closer than previous
	// in order to change target, but, reset factor to 1.0f in case we lock and check same type
	// objects (or don't have any target yet)
	float tmpDistanceFactorByObjectType{1.0f};

	ForEachSpaceShip([&] (const cSpaceShip &tmpShip) {
		// проверка, чтобы не считать свой корабль
		if ((NeedCheckCollision(tmpShip)) &&
		    ObjectsStatusFoe(ObjectStatus, tmpShip.ObjectStatus)) {
			// находим настоящую точку попадания с учетом скорости объекта и пули... если надо
			sVECTOR3D tmpLocation = tmpShip.GeometryCenter;
			vw_Matrix33CalcPoint(tmpLocation, tmpShip.CurrentRotationMat); // поворачиваем в плоскость объекта
			sVECTOR3D RealLocation = tmpShip.Location + tmpLocation;

			if ((tmpShip.Speed != 0.0f) &&
			    (WeaponType != 0) &&
			    // это не лучевое оружие, которое бьет сразу
			    (WeaponType != 11) && (WeaponType != 12) && (WeaponType != 14) &&
			    // это не ракеты...
			    (WeaponType != 16) && (WeaponType != 17) && (WeaponType != 18) && (WeaponType != 19)) {

				// находим, за какое время снаряд долетит до объекта сейчас
				sVECTOR3D TTT = WeponLocation - RealLocation;
				float ProjectileSpeed = GetProjectileSpeed(WeaponType);
				float CurrentDist = TTT.Length();
				float ObjCurrentTime = CurrentDist / ProjectileSpeed;

				// находим где будет объект, когда пройдет это время
				sVECTOR3D FutureLocation = tmpShip.Orientation ^ (tmpShip.Speed * ObjCurrentTime);

				// находи точку по середине... это нам и надо... туда целимся...
				RealLocation = RealLocation + FutureLocation;
			}

			// ограничение на зону прицеливания, целиться только если цель находится напротив оружия
			if ((fabs(A * RealLocation.x +
				  B * RealLocation.y +
				  C * RealLocation.z + D) <= tmpShip.Radius) &&
			    // проверяем, спереди или сзади стоит противник
			    ((A2 * RealLocation.x +
			      B2 * RealLocation.y +
			      C2 * RealLocation.z + D2) > MinDistance)) {

				// выбираем объект, так, чтобы он был наиболее длижайшим,
				// идущим по нашему курсу...
				sVECTOR3D TargetAngleTMP;
				sVECTOR3D TargetLocation = RealLocation;

				sVECTOR3D tmpDistance = TargetLocation - Location;
				if (NeedByWeaponOrientation)
					tmpDistance = TargetLocation - WeponLocation;
				float tmpLength2 = tmpDistance.x * tmpDistance.x +
						   tmpDistance.y * tmpDistance.y +
						   tmpDistance.z * tmpDistance.z;
				float tmpLength = vw_sqrtf(tmpLength2);

				TargetAngleTMP.x = CurrentObjectRotation.x;
				if ((tmpLength > 0.0f) && (A3B3C3D3NormalLength > 0.0f)) {
					// see "Angle between line and plane" (geometry) for more info about what we are doing here
					float tmpSineOfAngle = (A3 * tmpDistance.x + B3 * tmpDistance.y + C3 * tmpDistance.z) /
							       (tmpLength * A3B3C3D3NormalLength);
					vw_Clamp(tmpSineOfAngle, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
					TargetAngleTMP.x = CurrentObjectRotation.x - asinf(tmpSineOfAngle) * RadToDeg;
				}

				TargetAngleTMP.y = CurrentObjectRotation.y;
				if (NeedCenterOrientation &&
				    (tmpLength > 0.0f) && (A2B2C2D2NormalLength > 0.0f)) {
					// see "Angle between line and plane" (geometry) for more info about what we are doing here
					float tmpSineOfAngle = (A * tmpDistance.x + B * tmpDistance.y + C * tmpDistance.z) /
							       (tmpLength * A2B2C2D2NormalLength);
					vw_Clamp(tmpSineOfAngle, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
					TargetAngleTMP.y = CurrentObjectRotation.y - asinf(tmpSineOfAngle) * RadToDeg;
				}

				TargetAngleTMP.z = CurrentObjectRotation.z;

				if ((tmpDistanceToLockedTarget2 / tmpDistanceFactorByObjectType > tmpLength2) &&
				    (fabsf(TargetAngleTMP.x) < 45.0f)) {
					NeedAngle = TargetAngleTMP;
					tmpDistanceToLockedTarget2 = tmpLength2;
					TargetLocked = true;
					tmpDistanceFactorByObjectType = 1.0f;
				}
			}
		}
	});

	// проверка по наземным объектам
	// не стрелять по "мирным" постойкам
	// !!! ВАЖНО
	// у всех наземных объектов ноль на уровне пола...
	if (TargetLocked)
		tmpDistanceFactorByObjectType = 5.0f;
	ForEachGroundObject([&] (const cGroundObject &tmpGround) {
		// если по этому надо стрелять
		if (NeedCheckCollision(tmpGround) &&
		    ObjectsStatusFoe(ObjectStatus, tmpGround.ObjectStatus)) {

			sVECTOR3D tmpLocation = tmpGround.GeometryCenter;
			vw_Matrix33CalcPoint(tmpLocation, tmpGround.CurrentRotationMat); // поворачиваем в плоскость объекта
			sVECTOR3D RealLocation = tmpGround.Location + tmpLocation;

			if ((tmpGround.Speed != 0.0f) &&
			    (WeaponType != 0) &&
			    // это не лучевое оружие, которое бьет сразу
			    (WeaponType != 11) && (WeaponType != 12) && (WeaponType != 14) &&
			    // это не ракеты...
			    (WeaponType != 16) && (WeaponType != 17) && (WeaponType != 18) && (WeaponType != 19)) {

				// находим, за какое время снаряд долетит до объекта сейчас
				sVECTOR3D TTT = WeponLocation - RealLocation;
				float ProjectileSpeed = GetProjectileSpeed(WeaponType);
				float CurrentDist = TTT.Length();
				float ObjCurrentTime = CurrentDist / ProjectileSpeed;

				// находим где будет объект, когда пройдет это время (+ сразу половину считаем!)
				sVECTOR3D FutureLocation = tmpGround.Orientation ^ (tmpGround.Speed * ObjCurrentTime);

				// находи точку по середине... это нам и надо... туда целимся...
				RealLocation = RealLocation + FutureLocation;
			}

			// ограничение на зону прицеливания, целиться только если цель находится напротив оружия
			if ((fabs(A * RealLocation.x +
				  B * RealLocation.y +
				  C * RealLocation.z + D) <= tmpGround.Radius) &&
			    // проверяем, спереди или сзади стоит противник
			    ((A2 * RealLocation.x +
			      B2 * RealLocation.y +
			      C2 * RealLocation.z + D2) > MinDistance)) {

				// выбираем объект, так, чтобы он был наиболее длижайшим,
				// идущим по нашему курсу...
				sVECTOR3D TargetAngleTMP;
				sVECTOR3D TargetLocation = RealLocation;

				sVECTOR3D tmpDistance = TargetLocation - Location;
				if (NeedByWeaponOrientation)
					tmpDistance = TargetLocation - WeponLocation;
				float tmpLength2 = tmpDistance.x * tmpDistance.x +
						   tmpDistance.y * tmpDistance.y +
						   tmpDistance.z * tmpDistance.z;
				float tmpLength = vw_sqrtf(tmpLength2);

				TargetAngleTMP.x = CurrentObjectRotation.x;
				if ((tmpLength > 0.0f) && (A3B3C3D3NormalLength > 0.0f)) {
					// see "Angle between line and plane" (geometry) for more info about what we are doing here
					float tmpSineOfAngle = (A3 * tmpDistance.x + B3 * tmpDistance.y + C3 * tmpDistance.z) /
							       (tmpLength * A3B3C3D3NormalLength);
					vw_Clamp(tmpSineOfAngle, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
					TargetAngleTMP.x = CurrentObjectRotation.x - asinf(tmpSineOfAngle) * RadToDeg;
				}

				TargetAngleTMP.y = CurrentObjectRotation.y;
				if (NeedCenterOrientation &&
				    (tmpLength > 0.0f) && (A2B2C2D2NormalLength > 0.0f)) {
					// see "Angle between line and plane" (geometry) for more info about what we are doing here
					float tmpSineOfAngle = (A * tmpDistance.x + B * tmpDistance.y + C * tmpDistance.z) /
							       (tmpLength * A2B2C2D2NormalLength);
					vw_Clamp(tmpSineOfAngle, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
					TargetAngleTMP.y = CurrentObjectRotation.y - asinf(tmpSineOfAngle) * RadToDeg;
				}

				TargetAngleTMP.z = CurrentObjectRotation.z;

				if ((tmpDistanceToLockedTarget2 / tmpDistanceFactorByObjectType > tmpLength2) &&
				    (fabsf(TargetAngleTMP.x) < 45.0f)) {
					NeedAngle = TargetAngleTMP;
					tmpDistanceToLockedTarget2 = tmpLength2;
					TargetLocked = true;
					tmpDistanceFactorByObjectType = 1.0f;
				}
			}
		}
	});

	// проверка по космическим объектам
	if (TargetLocked)
		tmpDistanceFactorByObjectType = 10.0f;
	ForEachSpaceObject([&] (const cSpaceObject &tmpSpace) {
		// если по этому надо стрелять
		if (NeedCheckCollision(tmpSpace) &&
		    ObjectsStatusFoe(ObjectStatus, tmpSpace.ObjectStatus)) {

			sVECTOR3D tmpLocation = tmpSpace.GeometryCenter;
			vw_Matrix33CalcPoint(tmpLocation, tmpSpace.CurrentRotationMat); // поворачиваем в плоскость объекта
			sVECTOR3D RealLocation = tmpSpace.Location + tmpLocation;

			// если нужно проверить
			if ((tmpSpace.Speed != 0.0f) &&
			    (WeaponType != 0) &&
			    // это не лучевое оружие, которое бьет сразу
			    (WeaponType != 11) && (WeaponType != 12) && (WeaponType != 14) &&
			    // это не ракеты...
			    (WeaponType != 16) && (WeaponType != 17) && (WeaponType != 18) && (WeaponType != 19)) {

				// находим, за какое время снаряд долетит до объекта сейчас
				sVECTOR3D TTT = WeponLocation - RealLocation;
				float ProjectileSpeed = GetProjectileSpeed(WeaponType);
				float CurrentDist = TTT.Length();
				float ObjCurrentTime = CurrentDist / ProjectileSpeed;

				// находим где будет объект, когда пройдет это время (+ сразу половину считаем!)
				sVECTOR3D FutureLocation = tmpSpace.Orientation ^ (tmpSpace.Speed * ObjCurrentTime);

				// находи точку по середине... это нам и надо... туда целимся...
				RealLocation = RealLocation + FutureLocation;
			}

			// ограничение на зону прицеливания, целиться только если цель находится напротив оружия
			if ((fabs(A * RealLocation.x +
				  B * RealLocation.y +
				  C * RealLocation.z + D) <= tmpSpace.Radius) &&
			    // проверяем, спереди или сзади стоит противник
			    ((A2 * RealLocation.x +
			      B2 * RealLocation.y +
			      C2 * RealLocation.z + D2) > MinDistance)) {

				// выбираем объект, так, чтобы он был наиболее длижайшим,
				// идущим по нашему курсу...
				sVECTOR3D TargetAngleTMP;
				sVECTOR3D TargetLocation = RealLocation;

				sVECTOR3D tmpDistance = TargetLocation - Location;
				if (NeedByWeaponOrientation)
					tmpDistance = TargetLocation - WeponLocation;
				float tmpLength2 = tmpDistance.x * tmpDistance.x +
						   tmpDistance.y * tmpDistance.y +
						   tmpDistance.z * tmpDistance.z;
				float tmpLength = vw_sqrtf(tmpLength2);

				TargetAngleTMP.x = CurrentObjectRotation.x;
				if ((tmpLength > 0.0f) && (A3B3C3D3NormalLength > 0.0f)) {
					// see "Angle between line and plane" (geometry) for more info about what we are doing here
					float tmpSineOfAngle = (A3 * tmpDistance.x + B3 * tmpDistance.y + C3 * tmpDistance.z) /
							       (tmpLength * A3B3C3D3NormalLength);
					vw_Clamp(tmpSineOfAngle, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
					TargetAngleTMP.x = CurrentObjectRotation.x - asinf(tmpSineOfAngle) * RadToDeg;
				}

				TargetAngleTMP.y = CurrentObjectRotation.y;
				if (NeedCenterOrientation &&
				    (tmpLength > 0.0f) && (A2B2C2D2NormalLength > 0.0f)) {
					// see "Angle between line and plane" (geometry) for more info about what we are doing here
					float tmpSineOfAngle = (A * tmpDistance.x + B * tmpDistance.y + C * tmpDistance.z) /
							       (tmpLength * A2B2C2D2NormalLength);
					vw_Clamp(tmpSineOfAngle, -1.0f, 1.0f); // arc sine is computed in the interval [-1, +1]
					TargetAngleTMP.y = CurrentObjectRotation.y - asinf(tmpSineOfAngle) * RadToDeg;
				}

				TargetAngleTMP.z = CurrentObjectRotation.z;

				if ((tmpDistanceToLockedTarget2 / tmpDistanceFactorByObjectType > tmpLength2) &&
				    (fabsf(TargetAngleTMP.x) < 45.0f)) {
					NeedAngle = TargetAngleTMP;
					tmpDistanceToLockedTarget2 = tmpLength2;
					TargetLocked = true;
					tmpDistanceFactorByObjectType = 1.0f;
				}
			}
		}
	});
}

} // astromenace namespace
} // viewizard namespace
