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

// FIXME we use "strength", "hull" and "armour" in the same meaning, switch to "armor"

// TODO codestyle should be fixed

// TODO translate comments

#include "object3d.h"
#include "collision_detection.h"
#include "../config/config.h"
#include "../assets/audio.h"
#include "space_ship/space_ship.h"
#include "ground_object/ground_object.h"
#include "projectile/projectile.h"
#include "space_object/space_object.h"
#include "explosion/explosion.h"

// NOTE switch to nested namespace definition (namespace A::B::C { ... }) (since C++17)
namespace viewizard {
namespace astromenace {

// FIXME should be fixed, don't allow global scope interaction for local variables
extern float GameMoney;
extern float GameExperience;
extern int CurrentProfile;
extern int AlienShipsKillQuant;
extern float AlienShipsKillBonus;
extern int AlienMotherShipsKillQuant;
extern float AlienMotherShipsKillBonus;
extern int PirateShipsKillQuant;
extern float PirateShipsKillBonus;
extern int PirateVehiclesKillQuant;
extern float PirateVehiclesKillBonus;
extern int PirateBuildingsKillQuant;
extern float PirateBuildingsKillBonus;
extern int AsteroidsKillQuant;
extern float AsteroidsKillBonus;

extern int CurrentMission;
float GameCameraGetSpeed();

extern float ShildEnergyStatus;
extern float ShildStartHitStatus;
extern int PlayerDeadObjectPieceNum;


/*
 * Calculate bonus and experience for killed enemy.
 */
static void AddBonusForKilledEnemy(const cObject3D &Object, eObjectStatus KilledBy)
{
	if ((KilledBy != eObjectStatus::Player) ||
	    (Object.ObjectStatus != eObjectStatus::Enemy) ||
	    (Object.StrengthStart <= 0.0f))
		return;

	// TODO probably, we should calculate "static part" on mission start only
	float tmpBonus = Object.StrengthStart * GameEnemyArmorPenalty /
			 (2.0f * GameConfig().Profile[CurrentProfile].MissionReplayCount[CurrentMission]);
	float tmpExperience = Object.StrengthStart * GameEnemyArmorPenalty * ProfileDifficulty(CurrentProfile) / 100.0f;

	switch (Object.ObjectType) {
	case eObjectType::AlienFighter:
		tmpBonus /= 1.8f;
		tmpExperience /= 1.8f;
		AlienShipsKillBonus += tmpBonus;
		AlienShipsKillQuant++;
		break;

	case eObjectType::AlienMotherShip:
		tmpBonus /= 1.8f;
		tmpExperience /= 1.8f;
		AlienMotherShipsKillBonus += tmpBonus;
		AlienMotherShipsKillQuant++;
		break;

	case eObjectType::PirateShip:
		tmpBonus /= 1.8f;
		tmpExperience /= 1.8f;
		PirateShipsKillBonus += tmpBonus;
		PirateShipsKillQuant++;
		break;

	case eObjectType::PirateVehicle:
		tmpBonus /= 1.8f;
		tmpExperience /= 1.8f;
		PirateVehiclesKillBonus += tmpBonus;
		PirateVehiclesKillQuant++;
		break;

	case eObjectType::PirateBuilding:
		tmpBonus /= 1.8f;
		tmpExperience /= 1.8f;
		PirateBuildingsKillBonus += tmpBonus;
		PirateBuildingsKillQuant++;
		break;

	case eObjectType::SmallAsteroid:
		tmpBonus /= 8.0f;
		tmpExperience /= 8.0f;
		AsteroidsKillBonus += tmpBonus;
		AsteroidsKillQuant++;
		break;

#ifndef NDEBUG
	// don't use 'default' case here, we need compiler's warning if some type was missed
	case eObjectType::none:
	case eObjectType::EarthFighter:
	case eObjectType::SpaceDebris:
	case eObjectType::ShipWeapon:
	case eObjectType::Explosion:
	case eObjectType::CivilianBuilding:
	case eObjectType::BasePart:
	case eObjectType::Planet:
	case eObjectType::Planetoid:
	case eObjectType::BigAsteroid:
	case eObjectType::Projectile:
		return;
#else
	default:
		return;
#endif // NDEBUG
	}

	GameMoney += tmpBonus;
	GameExperience += tmpExperience;
}

/*
 * Setup space object explosion.
 */
static void SetupSpaceExplosion(cSpaceObject &SpaceObject)
{
	switch (SpaceObject.ObjectType) {
	case eObjectType::SmallAsteroid:
		CreateSpaceExplosion(SpaceObject, 1, SpaceObject.Location, SpaceObject.Speed, -1);
		break;
	case eObjectType::SpaceDebris:
		CreateSpaceExplosion(SpaceObject, 32, SpaceObject.Location, SpaceObject.Speed, -1);
		break;
	default:
		break;
	}
}

/*
 * Setup space ship explosion.
 */
static void SetupSpaceShipExplosion(cSpaceShip &SpaceShip, int ObjectChunkNum)
{
	switch (SpaceShip.ObjectType) {
	case eObjectType::AlienFighter:
		CreateSpaceExplosion(SpaceShip, 2, SpaceShip.Location, SpaceShip.Speed, ObjectChunkNum);
		break;
	case eObjectType::EarthFighter:
		CreateSpaceExplosion(SpaceShip, 31, SpaceShip.Location, SpaceShip.Speed, ObjectChunkNum);
		break;
	case eObjectType::AlienMotherShip:
		if (ObjectChunkNum == -1) // FIXME do we really need this?
			ObjectChunkNum = 0;
		CreateSpaceExplosion(SpaceShip, 33, SpaceShip.Location, SpaceShip.Speed, ObjectChunkNum);
		break;
	case eObjectType::PirateShip:
		if (SpaceShip.InternalType <= 5)
			CreateSpaceExplosion(SpaceShip, 3, SpaceShip.Location, SpaceShip.Speed, ObjectChunkNum);
		else {
			if (ObjectChunkNum == -1) // FIXME do we really need this?
				ObjectChunkNum = 0;
			CreateSpaceExplosion(SpaceShip, 31, SpaceShip.Location, SpaceShip.Speed, ObjectChunkNum);
		}
		break;
	default:
		break;
	}
}

/*
 * Setup ground object explosion.
 */
static void SetupGroundExplosion(cGroundObject &GroundObject, int ObjectChunkNum)
{
	switch (GroundObject.ObjectType) {
	case eObjectType::PirateBuilding:
		CreateGroundExplosion(GroundObject, 1, GroundObject.Location, ObjectChunkNum);
		break;
	case eObjectType::PirateVehicle:
		CreateGroundExplosion(GroundObject, 2, GroundObject.Location, ObjectChunkNum);
		break;
	default:
		break;
	}
}

/*
 * Check distance between points in space.
 * Return square of the distance factor for future calculations.
 */
static bool CheckDistanceBetweenPoints(const sVECTOR3D &Point1, const sVECTOR3D &Point2,
				       float Distance2, float &Distance2Factor)
{
	float RealDistance2 = (Point1.x - Point2.x) * (Point1.x - Point2.x) +
			      (Point1.y - Point2.y) * (Point1.y - Point2.y) +
			      (Point1.z - Point2.z) * (Point1.z - Point2.z);

	if (RealDistance2 <= Distance2) {
		Distance2Factor = RealDistance2 / Distance2;
		return true;
	}

	// we don't need real square of the distance factor in this case
	Distance2Factor = 1.0f;
	return false;
}

/*
 * Damage all near objects by shock wave.
 */
static void DamageAllNearObjectsByShockWave(const cObject3D &DontTouchObject, const sVECTOR3D &Epicenter,
					    float Radius2, float Damage, eObjectStatus ExplosionStatus)
{
	// FIXME
	// we don't destroy projectiles (missiles/bombs/torpedos) since we could have an issue with
	// ForEachProjectile()/ForEachProjectilePair(), just make sure you don't destroy bomb/torpedo
	// itself here, but not only DontTouchObject

	// reduce shock wave damage to 75%, let bomb's/torpedo's hit damage more than shock wave
	Damage = Damage * 0.75f;
	// we need take into account distance factor for damage calculation
	float Distance2Factor;

	ForEachSpaceObject([&] (cSpaceObject &tmpSpace, eSpaceCycle &SpaceCycleCommand) {
		if (NeedCheckCollision(tmpSpace) &&
		    ObjectsStatusFoe(ExplosionStatus, tmpSpace.ObjectStatus) &&
		    (&DontTouchObject != &tmpSpace) &&
		    CheckDistanceBetweenPoints(tmpSpace.Location, Epicenter, Radius2, Distance2Factor)) {
			// debris is a part of scene, don't let them all explode by only one shock wave
			if ((tmpSpace.ObjectType == eObjectType::SpaceDebris) &&
			    (vw_fRand() > 0.5f))
				return; // eSpaceCycle::Continue;

			tmpSpace.Strength -= Damage * (1.0f - Distance2Factor);

			if (tmpSpace.Strength <= 0.0f) {
				AddBonusForKilledEnemy(tmpSpace, ExplosionStatus);
				SetupSpaceExplosion(tmpSpace);
				SpaceCycleCommand = eSpaceCycle::DeleteObjectAndContinue;
			}
		}
	});

	ForEachSpaceShip([&] (cSpaceShip &tmpShip, eShipCycle &ShipCycleCommand) {
		if (NeedCheckCollision(tmpShip) &&
		    ObjectsStatusFoe(ExplosionStatus, tmpShip.ObjectStatus) &&
		    (&DontTouchObject != &tmpShip) &&
		    CheckDistanceBetweenPoints(tmpShip.Location, Epicenter, Radius2, Distance2Factor)) {

			tmpShip.ShieldStrength = 0.0f; // EMP with bomb/torpedo explosion should reduce shields to 0
			tmpShip.Strength -= Damage * (1.0f - Distance2Factor);

			if ((tmpShip.Strength <= 0.0f) &&
			    (tmpShip.ObjectStatus != eObjectStatus::Player)) {
				AddBonusForKilledEnemy(tmpShip, ExplosionStatus);
				SetupSpaceShipExplosion(tmpShip, -1);
				ShipCycleCommand = eShipCycle::DeleteObjectAndContinue;
			}
		}
	});

	ForEachGroundObject([&] (cGroundObject &tmpGround, eGroundCycle &GroundCycleCommand) {
		if (NeedCheckCollision(tmpGround) &&
		    ObjectsStatusFoe(ExplosionStatus, tmpGround.ObjectStatus) &&
		    (&DontTouchObject != &tmpGround) &&
		    CheckDistanceBetweenPoints(tmpGround.Location, Epicenter, Radius2, Distance2Factor)) {

			tmpGround.Strength -= Damage * (1.0f - Distance2Factor);

			if (tmpGround.Strength <= 0.0f) {
				AddBonusForKilledEnemy(tmpGround, ExplosionStatus);
				SetupGroundExplosion(tmpGround, -1);
				GroundCycleCommand = eGroundCycle::DeleteObjectAndContinue;
			}
		}
	});
}

/*
 * Detect projectile collision.
 */
bool DetectProjectileCollision(const cObject3D &Object, int &ObjectPieceNum, cProjectile &Projectile,
			       sVECTOR3D &IntercPoint, cDamage &Damage, float ObjectSpeed)
{
	if (!ObjectsStatusFoe(Object.ObjectStatus, Projectile.ObjectStatus) &&
	    NeedCheckCollision(Object))
		return false;

	// поправка на скорость камеры для корабля игрока
	if (Object.ObjectStatus == eObjectStatus::Player)
		ObjectSpeed += GameCameraGetSpeed();

	switch (Projectile.ProjectileType) {
	case 0: // projectile
		// если игрок со щитом или дифлектором, и щит заряжен
		if (((ShildEnergyStatus * ShildStartHitStatus) >= (Projectile.Damage.Full() * GameEnemyWeaponPenalty)) &&
		    (Object.ObjectStatus == eObjectStatus::Player) &&
			// у игрока есть щит, просто проверяем, если снаряд приблизился
			// на расстояние =< радиуса - уничтожаем его
		    vw_SphereSphereCollision(Object.Radius, Object.Location,
					     Projectile.Radius, Projectile.Location, Projectile.PrevLocation)) {
			// если отражатель, разворачиваем пулю...
			if (GameAdvancedProtectionSystem == 4) {
				Projectile.ObjectStatus = Object.ObjectStatus;
				Projectile.SetRotation(Projectile.Rotation ^ (-1));

				for (auto &tmpGFX : Projectile.GraphicFX) {
					if (auto sharedGFX = tmpGFX.lock()) {
						sharedGFX->ParticlesPerSec = static_cast<int>(sharedGFX->ParticlesPerSec * GameEnemyWeaponPenalty);
						sharedGFX->Speed = sharedGFX->Speed * GameEnemyWeaponPenalty;
						sharedGFX->Life = sharedGFX->Life / GameEnemyWeaponPenalty;
						sharedGFX->MagnetFactor = sharedGFX->MagnetFactor * GameEnemyWeaponPenalty * GameEnemyWeaponPenalty;
					}
				}
				Projectile.SpeedStart = Projectile.Speed * GameEnemyWeaponPenalty;
				Projectile.SpeedEnd = (Projectile.Speed * GameEnemyWeaponPenalty) / 4.0f;
				Projectile.Age = Projectile.Age / GameEnemyWeaponPenalty;
				Projectile.Lifetime = Projectile.Lifetime / GameEnemyWeaponPenalty;

				// корректируем данные щита
				float CurrentStatus = ShildEnergyStatus * ShildStartHitStatus;
				// физический урон минимален
				CurrentStatus -= Projectile.Damage.Kinetic() / 5.0f;
				// а ионный на оборот
				CurrentStatus -= Projectile.Damage.EM() * 2.0f;
				if (CurrentStatus < 0.0f)
					CurrentStatus = 0.0f;
				// и вычисляем сколько осталось в щите
				ShildEnergyStatus = CurrentStatus / ShildStartHitStatus;

				Projectile.Damage *= GameEnemyWeaponPenalty;

				// здесь только так! иначе уничтожим снаряд
				return false;
			} else {
				CreateBulletExplosion(&Object, Projectile, Projectile.Num, Projectile.Location, ObjectSpeed);

				// где сейчас, там и погибли
				IntercPoint = Projectile.Location;

				// корректируем данные щита
				float CurrentStatus = ShildEnergyStatus * ShildStartHitStatus;
				// физический урон минимален
				CurrentStatus -= Projectile.Damage.Kinetic() / 5.0f;
				// а ионный на оборот
				CurrentStatus -= Projectile.Damage.EM() * 2.0f;
				if (CurrentStatus < 0.0f)
					CurrentStatus = 0.0f;
				// и вычисляем сколько осталось в щите
				ShildEnergyStatus = CurrentStatus / ShildStartHitStatus;

				// столкновение не было (!!! именно так, иначе ерунда с указателем на снаряд)
				Damage = 0.0f;
				return true;
			}
		} else if (vw_SphereSphereCollision(Object.Radius, Object.Location,
						    Projectile.Radius, Projectile.Location, Projectile.PrevLocation) &&
			   vw_SphereAABBCollision(Object.AABB, Object.Location,
						  Projectile.Radius, Projectile.Location, Projectile.PrevLocation) &&
			   vw_SphereOBBCollision(Object.OBB.Box, Object.OBB.Location, Object.Location, Object.CurrentRotationMat,
						 Projectile.Radius, Projectile.Location, Projectile.PrevLocation) &&
			   CheckMeshSphereCollisionDetection(Object, Projectile, IntercPoint, ObjectPieceNum)) {

			if (NeedCheckCollision(Object)) {
				Damage = Projectile.Damage;
				CreateBulletExplosion(&Object, Projectile, Projectile.Num, IntercPoint, ObjectSpeed);
			} else {
				Damage = 0.0f;
				CreateBulletExplosion(&Object, Projectile, Projectile.Num, IntercPoint, 0.0f);
			}
			return true;
		}
		break;

	case 1: // projectile with 3d model
		// если игрок со щитом или дифлектором, и щит заряжен
		if (((ShildEnergyStatus * ShildStartHitStatus) >= (Projectile.Damage.Full() * GameEnemyWeaponPenalty)) &&
		    (Object.ObjectStatus == eObjectStatus::Player)) {
			// у игрока есть щит, просто проверяем, если снаряд приблизился
			// на расстояние =< радиуса - уничтожаем его
			if (vw_SphereSphereCollision(Object.Radius, Object.Location,
						     Projectile.Radius, Projectile.Location, Projectile.PrevLocation)) {
				IntercPoint = Projectile.Location;

				// "разбиваем" снаряд о корпус
				CreateBulletExplosion(&Object, Projectile, -Projectile.Num, Projectile.Location, ObjectSpeed);

				// корректируем данные щита
				float CurrentStatus = ShildEnergyStatus * ShildStartHitStatus;
				// физический урон минимален
				CurrentStatus -= Projectile.Damage.Kinetic() / 5.0f;
				// а ионный на оборот
				CurrentStatus -= Projectile.Damage.EM() * 2.0f;
				if (CurrentStatus < 0.0f)
					CurrentStatus = 0.0f;
				// и вычисляем сколько осталось в щите
				ShildEnergyStatus = CurrentStatus / ShildStartHitStatus;

				// передаем, столкновение было, чтобы корректно удалить снаряд в общей процедуре
				Damage = 0.0f;
				return true;
			}
		} else if (vw_SphereSphereCollision(Object.Radius, Object.Location,
						    Projectile.Radius, Projectile.Location, Projectile.PrevLocation) &&
			   // это ракета, просто проверяем, что она близко - и взрываем
			   vw_SphereAABBCollision(Object.AABB, Object.Location,
						  Projectile.Radius, Projectile.Location, Projectile.PrevLocation) &&
			   vw_SphereOBBCollision(Object.OBB.Box, Object.OBB.Location, Object.Location, Object.CurrentRotationMat,
						 Projectile.Radius, Projectile.Location, Projectile.PrevLocation) &&
			// ставим так, т.к.на больших кораблях плохо
			   vw_OBBOBBCollision(Object.OBB.Box, Object.OBB.Location, Object.Location, Object.CurrentRotationMat,
					      Projectile.OBB.Box, Projectile.OBB.Location, Projectile.Location, Projectile.CurrentRotationMat) &&
			   CheckHitBBOBBCollisionDetection(Object, Projectile, ObjectPieceNum) &&
			   CheckMeshSphereCollisionDetection(Object, Projectile, IntercPoint, ObjectPieceNum)) {

			switch (Projectile.Num) {
			case 18: // torpedo
			case 209: // pirate torpedo
				DamageAllNearObjectsByShockWave(Object, Projectile.Location, 75.0f * 75.0f,
								Projectile.Damage.Kinetic(), Projectile.ObjectStatus);
				break;
			case 19: // bomb
			case 210: // pirate bomb
				DamageAllNearObjectsByShockWave(Object, Projectile.Location, 150.0f * 150.0f,
								Projectile.Damage.Kinetic(), Projectile.ObjectStatus);
				break;
			default:
				break;
			}

			IntercPoint = Projectile.Location;

			if (NeedCheckCollision(Object)) {
				Damage = Projectile.Damage;
				CreateBulletExplosion(&Object, Projectile, Projectile.Num, Projectile.Location, Projectile.Speed);
			} else {
				Damage = 0.0f;
				CreateBulletExplosion(&Object, Projectile, Projectile.Num, Projectile.Location, 0.0f);
			}
			return true;
		}
		break;

	case 2: // beam
		if (vw_AABBAABBCollision(Object.AABB, Object.Location, Projectile.AABB, Projectile.Location) &&
		// в данном случае именно Projectile на первом месте!!!
		    vw_SphereOBBCollision(Projectile.OBB.Box, Projectile.OBB.Location, Projectile.Location, Projectile.CurrentRotationMat,
					  Object.Radius, Object.Location, Object.PrevLocation) &&
		    vw_OBBOBBCollision(Projectile.OBB.Box, Projectile.OBB.Location, Projectile.Location, Projectile.CurrentRotationMat,
				       Object.OBB.Box, Object.OBB.Location, Object.Location, Object.CurrentRotationMat) &&
		    CheckHitBBOBBCollisionDetection(Object, Projectile, ObjectPieceNum)) {
			IntercPoint = Object.Location;

			Damage.Kinetic() = Projectile.Damage.Kinetic() * Object.TimeDelta;
			Damage.EM() = 0.0f;
			return true;
		}
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Главная функция проверки столкновения
//-----------------------------------------------------------------------------
void DetectCollisionAllObject3D()
{
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// проверка для всех кораблей
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	ForEachSpaceShip([] (cSpaceShip &tmpShip, eShipCycle &ShipCycleCommand) {
		ForEachProjectile([&tmpShip, &ShipCycleCommand] (cProjectile &tmpProjectile, eProjectileCycle &ProjectileCycleCommand) {
			sVECTOR3D IntercPoint;
			cDamage Damage;
			int ObjectPieceNum;

			if (DetectProjectileCollision(tmpShip, ObjectPieceNum, tmpProjectile, IntercPoint, Damage, tmpShip.Speed)) {
				// we don't provide any global "resistance" concept for all 3D objects,
				// reduce damage for player ship only in case of "a-b layer" installed
				if ((tmpShip.ObjectStatus == eObjectStatus::Player) &&
				    (GameAdvancedProtectionSystem == 2)) {
					Damage.Kinetic() /= 4.0f;
					Damage.EM() /= 5.0f;
				}

				if (tmpShip.ShieldStrength >= Damage.EM()) {
					tmpShip.ShieldStrength -= Damage.EM();
					Damage.EM() = 0.0f;
				} else {
					Damage.EM() -= tmpShip.ShieldStrength;
					tmpShip.ShieldStrength = 0.0f;
				}

				if (tmpShip.ShieldStrength >= Damage.Kinetic()) {
					tmpShip.ShieldStrength -= Damage.Kinetic();
					Damage.Kinetic() = 0.0f;
				} else {
					Damage.Kinetic() -= tmpShip.ShieldStrength;
					tmpShip.ShieldStrength = 0.0f;
				}

				if (tmpShip.ShieldStrength < 0.0f)
					tmpShip.ShieldStrength = 0.0f;

				tmpShip.Strength -= Damage.Kinetic();
				// let EM occasionally corrupt armor in some way
				tmpShip.Strength -= Damage.EM() * vw_fRand();

				// since AlienFighter is "energy", we have a chance kill it by EM
				if ((Damage.EM() > 0.0f) &&
				    (tmpShip.ObjectType == eObjectType::AlienFighter) &&
				    (vw_fRand() > 0.7f))
					tmpShip.Strength = 0.0f;

				if (tmpShip.Strength <= 0.0f) {
					AddBonusForKilledEnemy(tmpShip, tmpProjectile.ObjectStatus);
					// если не корабль игрока! его удалим сами
					if (tmpShip.ObjectStatus != eObjectStatus::Player) {
						SetupSpaceShipExplosion(tmpShip, ObjectPieceNum);
						ShipCycleCommand = eShipCycle::DeleteObjectAndContinue;
					} else {
						// запоминаем, что взорвалось
						PlayerDeadObjectPieceNum = ObjectPieceNum;
					}
				} else if ((tmpShip.ObjectStatus != eObjectStatus::Player) && // игроку тут ничего не делаем!.. с него хватит и щита
					// если это не босс уровня (Alien MotherShip)
					   (tmpShip.ObjectType != eObjectType::AlienMotherShip) &&
					// если нужно, смотрим что делать с системами
					   (Damage.EM() > 0.0f)) {

					float Rand = vw_fRand();
					// поправка на мощность выстрела
					float DR = Damage.EM() / 300.0f;

					// выводим из строя управляемость кораблем
					if (Rand > 0.6f - DR)
						tmpShip.MaxSpeed = tmpShip.MaxSpeed / 2.0f;
					if ((Rand > 0.3f - DR) && (Rand < 0.6f))
						tmpShip.MaxSpeedRotate = tmpShip.MaxSpeedRotate / 2.0f;

					// если есть фларес, есть шанс его вырубить
					if (!tmpShip.FlareWeaponSlots.empty() &&
					    (Rand > 0.5f - DR) && (Rand < 0.8f))
						tmpShip.FlareWeaponSlots.clear();
				}

				if (tmpProjectile.ProjectileType != 2)
					ProjectileCycleCommand = eProjectileCycle::DeleteObjectAndContinue;

				if (ShipCycleCommand == eShipCycle::DeleteObjectAndContinue) {
					// break projectile cycle
					switch (ProjectileCycleCommand) {
					case eProjectileCycle::Continue:
						ProjectileCycleCommand = eProjectileCycle::Break;
						break;
					case eProjectileCycle::DeleteObjectAndContinue:
						ProjectileCycleCommand = eProjectileCycle::DeleteObjectAndBreak;
						break;
					default:
						break;
					}
				}
			}
			if (ProjectileCycleCommand != eProjectileCycle::Continue)
				return;

			// проверка на попадание в оружие (только для игрока и если включено в настройках)
			// проверять только до OBB
			if ((tmpShip.ObjectStatus == eObjectStatus::Player) &&
			    (tmpProjectile.ObjectStatus == eObjectStatus::Enemy) &&
			    !GameUndestroyableWeapon && !tmpShip.WeaponSlots.empty()) {
				for (auto &tmpWeaponSlot : tmpShip.WeaponSlots) {
					if (auto sharedWeapon = tmpWeaponSlot.Weapon.lock()) {
						int ObjectPieceNumWeapon;

						if ((sharedWeapon->Strength > 0.0f) &&
						    DetectProjectileCollision(*sharedWeapon, ObjectPieceNumWeapon, tmpProjectile, IntercPoint, Damage, tmpShip.Speed)) {
							// FIXME вот тут все очень плохо, т.к. можем убить и сам tmpShip

							// просто делаем изменения в прочности... и больше ничего
							sharedWeapon->Strength -= Damage.Kinetic();
							if (sharedWeapon->Strength <= 0.0f) {
								sharedWeapon->Strength = 0.0f;
								PlayVoicePhrase(eVoicePhrase::WeaponDestroyed, 1.0f);
							} else
								PlayVoicePhrase(eVoicePhrase::WeaponDamaged, 1.0f);

							// удаляем только те, которые разбились
							if (tmpProjectile.ProjectileType != 2) {
								ProjectileCycleCommand = eProjectileCycle::DeleteObjectAndContinue;
								return;
							}
						}
					}
				}
			}
		});
		if (ShipCycleCommand == eShipCycle::DeleteObjectAndContinue)
			return;

		// проверяем столкновение
		// cSpaceObject
		ForEachSpaceObject([&tmpShip, &ShipCycleCommand] (cSpaceObject &tmpSpace, eSpaceCycle &SpaceCycleCommand) {
			int ObjectPieceNum;
			if (vw_SphereSphereCollision(tmpShip.Radius, tmpShip.Location,
						     tmpSpace.Radius, tmpSpace.Location, tmpSpace.PrevLocation) &&
			    vw_SphereAABBCollision(tmpShip.AABB, tmpShip.Location,
						   tmpSpace.Radius, tmpSpace.Location, tmpSpace.PrevLocation) &&
			    vw_SphereOBBCollision(tmpShip.OBB.Box, tmpShip.OBB.Location, tmpShip.Location, tmpShip.CurrentRotationMat,
						  tmpSpace.Radius, tmpSpace.Location, tmpSpace.PrevLocation) &&
			    CheckHitBBOBBCollisionDetection(tmpShip, tmpSpace, ObjectPieceNum)) {

				if (((tmpSpace.ObjectType == eObjectType::BasePart) ||
				     (tmpSpace.ObjectType == eObjectType::BigAsteroid)) &&
				    !CheckHitBBMeshCollisionDetection(tmpShip, tmpSpace, ObjectPieceNum))
					return; // eSpaceCycle::Continue

				// если столкновение с преградой которую не можем уничтожить
				if (!NeedCheckCollision(tmpSpace)) {
					// ум. из расчета, что полностью разрушим за 2 секунды для игрока и 0.5 секунду для остальных
					if (tmpShip.ObjectStatus != eObjectStatus::Player)
						tmpShip.Strength -= (tmpShip.StrengthStart / 0.5f) * tmpShip.TimeDelta;
					else
						tmpShip.Strength -= (tmpShip.StrengthStart / 2.0f) * tmpShip.TimeDelta;
				} else {
					float StrTMP = tmpShip.Strength;
					tmpShip.Strength -= tmpSpace.Strength;
					tmpSpace.Strength -= StrTMP;
				}
				if (!NeedCheckCollision(tmpShip))
					tmpSpace.Strength = 0.0f;


				// если уже все... удаляем
				if (NeedCheckCollision(tmpSpace) &&
				    (tmpSpace.Strength <= 0.0f)) {
					AddBonusForKilledEnemy(tmpSpace, tmpShip.ObjectStatus);
					SetupSpaceExplosion(tmpSpace);
					SpaceCycleCommand = eSpaceCycle::DeleteObjectAndContinue;
				}

				if (NeedCheckCollision(tmpShip) &&
					// если уже все... удаляем
				    (tmpShip.Strength <= 0.0f)) {
					// если не корабль игрока! его удалим сами
					if (tmpShip.ObjectStatus != eObjectStatus::Player) {
						SetupSpaceShipExplosion(tmpShip, ObjectPieceNum);
						ShipCycleCommand = eShipCycle::DeleteObjectAndContinue;

						// break space cycle
						switch (SpaceCycleCommand) {
						case eSpaceCycle::Continue:
							SpaceCycleCommand = eSpaceCycle::Break;
							break;
						case eSpaceCycle::DeleteObjectAndContinue:
							SpaceCycleCommand = eSpaceCycle::DeleteObjectAndBreak;
							break;
						default:
							break;
						}

					} else
						PlayerDeadObjectPieceNum = ObjectPieceNum; // запоминаем, что взорвалось
				}
			}
		});
		if (ShipCycleCommand == eShipCycle::DeleteObjectAndContinue)
			return;

		// проверяем на столкновение с наземнымы объектами
		// 1 - радиус-радиус
		// 2 - AABB-AABB
		// 3 - OBB-OBB
		// 4 - HitBB-HitBB
		ForEachGroundObject([&tmpShip, &ShipCycleCommand] (cGroundObject &tmpGround, eGroundCycle &GroundCycleCommand) {
			int ObjectPieceNum1;
			int ObjectPieceNum2;
			if (vw_SphereSphereCollision(tmpShip.Radius, tmpShip.Location,
						     tmpGround.Radius, tmpGround.Location, tmpGround.PrevLocation) &&
			    vw_SphereAABBCollision(tmpShip.AABB, tmpShip.Location,
						   tmpGround.Radius, tmpGround.Location, tmpGround.PrevLocation) &&
			    vw_SphereOBBCollision(tmpShip.OBB.Box, tmpShip.OBB.Location, tmpShip.Location, tmpShip.CurrentRotationMat,
						  tmpGround.Radius, tmpGround.Location, tmpGround.PrevLocation) &&
			    CheckHitBBHitBBCollisionDetection(tmpShip, tmpGround, ObjectPieceNum1, ObjectPieceNum2)) {

				if ((tmpGround.ObjectType == eObjectType::CivilianBuilding) &&
				    !CheckHitBBMeshCollisionDetection(tmpShip, tmpGround, ObjectPieceNum2))
						return; // eGroundCycle::Continue

				// если столкновение с преградой которую не можем уничтожить
				if (!NeedCheckCollision(tmpGround)) {
					// ум. из расчета, что полностью разрушим за 2 секунды для игрока и 0.5 секунду для остальных
					if (tmpShip.ObjectStatus != eObjectStatus::Player)
						tmpShip.Strength -= (tmpShip.StrengthStart / 0.5f) * tmpShip.TimeDelta;
					else
						tmpShip.Strength -= (tmpShip.StrengthStart / 2.0f) * tmpShip.TimeDelta;
				} else {
					float StrTMP = tmpShip.Strength;
					tmpShip.Strength -= tmpGround.Strength;
					tmpGround.Strength -= StrTMP;
				}
				if (!NeedCheckCollision(tmpShip))
					tmpGround.Strength = 0.0f;

				// если уже все... удаляем
				if (NeedCheckCollision(tmpGround) &&
				    (tmpGround.Strength <= 0.0f)) {
					AddBonusForKilledEnemy(tmpGround, tmpShip.ObjectStatus);
					SetupGroundExplosion(tmpGround, ObjectPieceNum2);
					GroundCycleCommand = eGroundCycle::DeleteObjectAndContinue;
				}

				if (NeedCheckCollision(tmpShip) &&
					// если уже все... удаляем
				    (tmpShip.Strength <= 0.0f)) {
					// если не корабль игрока! его удалим сами
					if (tmpShip.ObjectStatus != eObjectStatus::Player) {
						SetupSpaceShipExplosion(tmpShip, ObjectPieceNum1);
						ShipCycleCommand = eShipCycle::DeleteObjectAndContinue;

						// break ground cycle
						switch (GroundCycleCommand) {
						case eGroundCycle::Continue:
							GroundCycleCommand = eGroundCycle::Break;
							break;
						case eGroundCycle::DeleteObjectAndContinue:
							GroundCycleCommand = eGroundCycle::DeleteObjectAndBreak;
							break;
						default:
							break;
						}

					} else
						PlayerDeadObjectPieceNum = ObjectPieceNum1; // запоминаем, что взорвалось
				}
			}
		});
	});

	ForEachSpaceShipPair([] (cSpaceShip &FirstShip, cSpaceShip &SecondShip, eShipPairCycle &Command) {
		int ObjectPieceNum1;
		int ObjectPieceNum2;
		if (vw_SphereSphereCollision(FirstShip.Radius, FirstShip.Location,
					     SecondShip.Radius, SecondShip.Location, SecondShip.PrevLocation) &&
		    vw_AABBAABBCollision(FirstShip.AABB, FirstShip.Location, SecondShip.AABB, SecondShip.Location) &&
		    vw_OBBOBBCollision(FirstShip.OBB.Box, FirstShip.OBB.Location, FirstShip.Location, FirstShip.CurrentRotationMat,
				       SecondShip.OBB.Box, SecondShip.OBB.Location, SecondShip.Location, SecondShip.CurrentRotationMat) &&
		    CheckHitBBHitBBCollisionDetection(FirstShip, SecondShip, ObjectPieceNum1, ObjectPieceNum2)) {

			float StrTMP = FirstShip.Strength;
			FirstShip.Strength -= SecondShip.Strength;
			SecondShip.Strength -= StrTMP;

			// если столкновение с преградой которую не можем уничтожить
			if (!NeedCheckCollision(SecondShip))
				FirstShip.Strength = 0.0f;
			if (!NeedCheckCollision(FirstShip))
				SecondShip.Strength = 0.0f;

			// проверка на бонус
			if (NeedCheckCollision(SecondShip) &&
			    (SecondShip.Strength <= 0.0f))
				AddBonusForKilledEnemy(SecondShip, FirstShip.ObjectStatus);
			if (NeedCheckCollision(FirstShip) &&
			    (FirstShip.Strength <= 0.0f))
				AddBonusForKilledEnemy(FirstShip, SecondShip.ObjectStatus);

			if (NeedCheckCollision(SecondShip) &&
				// если уже все... удаляем
			    (SecondShip.Strength <= 0.0f)) {
				// если не корабль игрока! его удалим сами
				if (SecondShip.ObjectStatus != eObjectStatus::Player) {
					SetupSpaceShipExplosion(SecondShip, ObjectPieceNum2);
					Command = eShipPairCycle::DeleteSecondObjectAndContinue;
				} else
					PlayerDeadObjectPieceNum = ObjectPieceNum2; // запоминаем, что взорвалось
			}

			if (NeedCheckCollision(FirstShip) &&
				// если уже все... удаляем
			    (FirstShip.Strength <= 0.0f)) {
				// если не корабль игрока! его удалим сами
				if (FirstShip.ObjectStatus != eObjectStatus::Player) {
					SetupSpaceShipExplosion(FirstShip, ObjectPieceNum1);
					if (Command == eShipPairCycle::DeleteSecondObjectAndContinue)
						Command = eShipPairCycle::DeleteBothObjectsAndContinue;
					else
						Command = eShipPairCycle::DeleteFirstObjectAndContinue;
				} else
					PlayerDeadObjectPieceNum = ObjectPieceNum1; // запоминаем, что взорвалось
			}
		}
	});

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// проверяем все cGroundObject с
	// cProjectile
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	ForEachGroundObject([] (cGroundObject &tmpGround, eGroundCycle &GroundCycleCommand) {
		ForEachProjectile([&tmpGround, &GroundCycleCommand] (cProjectile &tmpProjectile, eProjectileCycle &ProjectileCycleCommand) {
			sVECTOR3D IntercPoint;
			cDamage Damage;
			int ObjectPieceNum;

			if (DetectProjectileCollision(tmpGround, ObjectPieceNum, tmpProjectile, IntercPoint, Damage, tmpGround.Speed)) {
				if (NeedCheckCollision(tmpGround)) {
					tmpGround.Strength -= Damage.Kinetic();

					// FIXME let EM occasionally corrupt armor in some way (see space ship code above)

					if (tmpGround.Strength <= 0.0f) {
						AddBonusForKilledEnemy(tmpGround, tmpProjectile.ObjectStatus);
						SetupGroundExplosion(tmpGround, ObjectPieceNum);
						GroundCycleCommand = eGroundCycle::DeleteObjectAndContinue;
					}
				}

				if (tmpProjectile.ProjectileType != 2)
					ProjectileCycleCommand = eProjectileCycle::DeleteObjectAndContinue;

				if (GroundCycleCommand == eGroundCycle::DeleteObjectAndContinue) {
					// break projectile cycle
					switch (ProjectileCycleCommand) {
					case eProjectileCycle::Continue:
						ProjectileCycleCommand = eProjectileCycle::Break;
						break;
					case eProjectileCycle::DeleteObjectAndContinue:
						ProjectileCycleCommand = eProjectileCycle::DeleteObjectAndBreak;
						break;
					default:
						break;
					}
				}
			}
		});
		if (GroundCycleCommand == eGroundCycle::DeleteObjectAndContinue)
			return;

		// проверяем столкновение
		// cSpaceObject
		ForEachSpaceObject([&tmpGround, &GroundCycleCommand] (cSpaceObject &tmpSpace, eSpaceCycle &SpaceCycleCommand) {
			int ObjectPieceNum;

			// не проверяем с частями базы
			if ((tmpSpace.ObjectType != eObjectType::BasePart) &&
				// не проверяем если оба не можем уничтожить
			    (NeedCheckCollision(tmpGround) || NeedCheckCollision(tmpSpace)) &&
			    vw_SphereSphereCollision(tmpGround.Radius, tmpGround.Location,
						    tmpSpace.Radius, tmpSpace.Location, tmpSpace.PrevLocation) &&
			    vw_SphereAABBCollision(tmpGround.AABB, tmpGround.Location,
						   tmpSpace.Radius, tmpSpace.Location, tmpSpace.PrevLocation) &&
			    vw_SphereOBBCollision(tmpGround.OBB.Box, tmpGround.OBB.Location, tmpGround.Location, tmpGround.CurrentRotationMat,
						  tmpSpace.Radius, tmpSpace.Location, tmpSpace.PrevLocation) &&
			    CheckHitBBOBBCollisionDetection(tmpGround, tmpSpace, ObjectPieceNum)) {

				// если столкновение с преградой которую не можем уничтожить
				if (!NeedCheckCollision(tmpSpace))
					tmpGround.Strength -= (tmpGround.StrengthStart / 0.5f) * tmpGround.TimeDelta;
				else {
					float StrTMP = tmpGround.Strength;
					tmpGround.Strength -= tmpSpace.Strength;
					tmpSpace.Strength -= StrTMP;
				}
				if (!NeedCheckCollision(tmpGround))
					tmpSpace.Strength = 0.0f;


				// если уже все... удаляем
				if (NeedCheckCollision(tmpSpace) &&
				    (tmpSpace.Strength <= 0.0f)) {
					AddBonusForKilledEnemy(tmpSpace, tmpGround.ObjectStatus);
					SetupSpaceExplosion(tmpSpace);
					SpaceCycleCommand = eSpaceCycle::DeleteObjectAndContinue;
				}

				if (NeedCheckCollision(tmpGround) &&
				    (tmpGround.Strength <= 0.0f)) {
					SetupGroundExplosion(tmpGround, ObjectPieceNum);
					GroundCycleCommand = eGroundCycle::DeleteObjectAndContinue;
					// break space cycle
					switch (SpaceCycleCommand) {
					case eSpaceCycle::Continue:
						SpaceCycleCommand = eSpaceCycle::Break;
						break;
					case eSpaceCycle::DeleteObjectAndContinue:
						SpaceCycleCommand = eSpaceCycle::DeleteObjectAndBreak;
						break;
					default:
						break;
					}
				}
			}
		});
	});

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// проверяем все cSpaceObject
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	ForEachSpaceObject([] (cSpaceObject &tmpSpace, eSpaceCycle &SpaceCycleCommand) {
		ForEachProjectile([&tmpSpace, &SpaceCycleCommand] (cProjectile &tmpProjectile, eProjectileCycle &ProjectileCycleCommand) {
			sVECTOR3D IntercPoint;
			cDamage Damage;
			int ObjectPieceNum;

			if (DetectProjectileCollision(tmpSpace, ObjectPieceNum, tmpProjectile, IntercPoint, Damage, tmpSpace.Speed)) {
				if (NeedCheckCollision(tmpSpace)) {
					tmpSpace.Strength -= Damage.Kinetic();

					// FIXME let EM occasionally corrupt armor in some way (see space ship code above)

					if (tmpSpace.Strength <= 0.0f) {
						AddBonusForKilledEnemy(tmpSpace, tmpProjectile.ObjectStatus);
						SetupSpaceExplosion(tmpSpace);
						SpaceCycleCommand = eSpaceCycle::DeleteObjectAndContinue;
					}
				}

				if (tmpProjectile.ProjectileType != 2)
					ProjectileCycleCommand = eProjectileCycle::DeleteObjectAndContinue;

				if (SpaceCycleCommand == eSpaceCycle::DeleteObjectAndContinue) {
					// break projectile cycle
					switch (ProjectileCycleCommand) {
					case eProjectileCycle::Continue:
						ProjectileCycleCommand = eProjectileCycle::Break;
						break;
					case eProjectileCycle::DeleteObjectAndContinue:
						ProjectileCycleCommand = eProjectileCycle::DeleteObjectAndBreak;
						break;
					default:
						break;
					}
				}
			}
		});
	});

	ForEachSpaceObjectPair([] (cSpaceObject &FirstObject, cSpaceObject &SecondObject, eSpacePairCycle &Command) {
		if ((NeedCheckCollision(SecondObject) || NeedCheckCollision(FirstObject)) &&
		    vw_SphereSphereCollision(FirstObject.Radius, FirstObject.Location,
					     SecondObject.Radius, SecondObject.Location, SecondObject.PrevLocation) &&
		    vw_OBBOBBCollision(FirstObject.OBB.Box, FirstObject.OBB.Location, FirstObject.Location, FirstObject.CurrentRotationMat,
				       SecondObject.OBB.Box, SecondObject.OBB.Location, SecondObject.Location, SecondObject.CurrentRotationMat)) {
			// если попали в часть базы - просто летим в другую сторону,
			// если это обломок корабля или модели
			if (((FirstObject.ObjectType == eObjectType::BasePart) && (SecondObject.ObjectType == eObjectType::SpaceDebris)) ||
			    ((FirstObject.ObjectType == eObjectType::SpaceDebris) && (SecondObject.ObjectType == eObjectType::BasePart))) {
				if (FirstObject.ObjectType == eObjectType::SpaceDebris)
					vw_RotatePoint(FirstObject.Orientation, sVECTOR3D{0.0f, 180.0f, 0.0f});
				if (SecondObject.ObjectType == eObjectType::SpaceDebris)
					vw_RotatePoint(SecondObject.Orientation, sVECTOR3D{0.0f, 180.0f, 0.0f});

				return; // eSpacePairCycle::Continue
			}

			// смотрим, чтобы это были не только обломки с обломками (иначе не красиво взрываются корабли)
			if ((SecondObject.ObjectType != eObjectType::SpaceDebris) ||
			    (FirstObject.ObjectType != eObjectType::SpaceDebris)) {

				int ObjectPieceNum;

				// проверка, если это столкновение с базой - надо внимательно смотреть
				if ((FirstObject.ObjectType == eObjectType::BasePart) &&
				    (!CheckHitBBMeshCollisionDetection(SecondObject, FirstObject, ObjectPieceNum)))
						return; // eSpacePairCycle::Continue
				// проверка, если это столкновение с базой - надо внимательно смотреть
				if ((SecondObject.ObjectType == eObjectType::BasePart) &&
				    (!CheckHitBBMeshCollisionDetection(FirstObject, SecondObject, ObjectPieceNum)))
						return; // eSpacePairCycle::Continue

				if ((NeedCheckCollision(SecondObject)) &&
				    ((SecondObject.ObjectType == eObjectType::SmallAsteroid) ||
				     (SecondObject.ObjectType == eObjectType::SpaceDebris))) {
					SetupSpaceExplosion(SecondObject);
					Command = eSpacePairCycle::DeleteSecondObjectAndContinue;
				}

				if (NeedCheckCollision(FirstObject) &&
				    ((FirstObject.ObjectType == eObjectType::SmallAsteroid) ||
				     (FirstObject.ObjectType == eObjectType::SpaceDebris))) {
					SetupSpaceExplosion(FirstObject);
					if (Command == eSpacePairCycle::DeleteSecondObjectAndContinue)
						Command = eSpacePairCycle::DeleteBothObjectsAndContinue;
					else
						Command = eSpacePairCycle::DeleteFirstObjectAndContinue;
				}
			}
		}
	});

	ForEachProjectilePair([] (cProjectile &FirstObject, cProjectile &SecondObject, eProjectilePairCycle &Command) {
		if (ObjectsStatusFoe(FirstObject.ObjectStatus, SecondObject.ObjectStatus)) {

			// missile/mine with missile/mine
			if (((FirstObject.ProjectileType == 1) || (FirstObject.ProjectileType == 4)) &&
			    ((SecondObject.ProjectileType == 1) || (SecondObject.ProjectileType == 4))) {
				if (vw_SphereSphereCollision(FirstObject.Radius, FirstObject.Location,
							     SecondObject.Radius, SecondObject.Location, SecondObject.PrevLocation) &&
				    vw_SphereAABBCollision(FirstObject.AABB, FirstObject.Location,
							   SecondObject.Radius, SecondObject.Location, SecondObject.PrevLocation) &&
				    vw_SphereOBBCollision(FirstObject.OBB.Box, FirstObject.OBB.Location, FirstObject.Location, FirstObject.CurrentRotationMat,
							  SecondObject.Radius, SecondObject.Location, SecondObject.PrevLocation) &&
				    vw_OBBOBBCollision(FirstObject.OBB.Box, FirstObject.OBB.Location, FirstObject.Location, FirstObject.CurrentRotationMat,
						       SecondObject.OBB.Box, SecondObject.OBB.Location, SecondObject.Location, SecondObject.CurrentRotationMat)) {

					if ((FirstObject.ProjectileType == 1) || (FirstObject.ProjectileType == 4)) {
						FirstObject.Speed = 0.0f;
						CreateBulletExplosion(nullptr, FirstObject, -FirstObject.Num, FirstObject.Location, FirstObject.Speed);
						Command = eProjectilePairCycle::DeleteFirstObjectAndContinue;
					}
					if ((SecondObject.ProjectileType == 1) || (SecondObject.ProjectileType == 4)) {
						SecondObject.Speed = 0.0f;
						CreateBulletExplosion(nullptr, SecondObject, -SecondObject.Num, SecondObject.Location, SecondObject.Speed);

						if (Command == eProjectilePairCycle::DeleteFirstObjectAndContinue)
							Command = eProjectilePairCycle::DeleteBothObjectsAndContinue;
						else
							Command = eProjectilePairCycle::DeleteSecondObjectAndContinue;
					}
				}
			// missile/mine with beam
			} else if (((FirstObject.ProjectileType == 1) || (FirstObject.ProjectileType == 4)) && (SecondObject.ProjectileType == 2)) {
				if (vw_SphereOBBCollision(SecondObject.OBB.Box, SecondObject.OBB.Location, SecondObject.Location, SecondObject.CurrentRotationMat,
							  FirstObject.Radius, FirstObject.Location, FirstObject.PrevLocation) &&
				    vw_OBBOBBCollision(FirstObject.OBB.Box, FirstObject.OBB.Location, FirstObject.Location, FirstObject.CurrentRotationMat,
						       SecondObject.OBB.Box, SecondObject.OBB.Location, SecondObject.Location, SecondObject.CurrentRotationMat)) {
					FirstObject.Speed = 0.0f;
					CreateBulletExplosion(nullptr, FirstObject, -FirstObject.Num, FirstObject.Location, FirstObject.Speed);
					Command = eProjectilePairCycle::DeleteFirstObjectAndContinue;
				}
			} else if (((SecondObject.ProjectileType == 1) || (SecondObject.ProjectileType == 4)) && (FirstObject.ProjectileType == 2)) {
				if (vw_SphereOBBCollision(FirstObject.OBB.Box, FirstObject.OBB.Location, FirstObject.Location, FirstObject.CurrentRotationMat,
							  SecondObject.Radius, SecondObject.Location, SecondObject.PrevLocation) &&
				    vw_OBBOBBCollision(FirstObject.OBB.Box, FirstObject.OBB.Location, FirstObject.Location, FirstObject.CurrentRotationMat,
						       SecondObject.OBB.Box, SecondObject.OBB.Location, SecondObject.Location, SecondObject.CurrentRotationMat)) {
					SecondObject.Speed = 0.0f;
					CreateBulletExplosion(nullptr, SecondObject, -SecondObject.Num, SecondObject.Location, SecondObject.Speed);
					Command = eProjectilePairCycle::DeleteSecondObjectAndContinue;
				}
			// missile/mine with projectile/flare
			} else if ((FirstObject.ProjectileType == 1) || (FirstObject.ProjectileType == 4)) {
				if (vw_SphereSphereCollision(FirstObject.Radius, FirstObject.Location,
							     SecondObject.Radius, SecondObject.Location, SecondObject.PrevLocation) &&
				    vw_SphereAABBCollision(FirstObject.AABB, FirstObject.Location,
							   SecondObject.Radius, SecondObject.Location, SecondObject.PrevLocation) &&
				    vw_SphereOBBCollision(FirstObject.OBB.Box, FirstObject.OBB.Location, FirstObject.Location, FirstObject.CurrentRotationMat,
							  SecondObject.Radius, SecondObject.Location, SecondObject.PrevLocation)) {
					FirstObject.Speed = 0.0f;
					CreateBulletExplosion(nullptr, FirstObject, -FirstObject.Num, FirstObject.Location, FirstObject.Speed);
					Command = eProjectilePairCycle::DeleteFirstObjectAndContinue;
				}
			} else if ((SecondObject.ProjectileType == 1) || (SecondObject.ProjectileType == 4)) {
				if (vw_SphereSphereCollision(SecondObject.Radius, SecondObject.Location,
							     FirstObject.Radius, FirstObject.Location, FirstObject.PrevLocation) &&
				    vw_SphereAABBCollision(SecondObject.AABB, SecondObject.Location,
							   FirstObject.Radius, FirstObject.Location, FirstObject.PrevLocation) &&
				    vw_SphereOBBCollision(SecondObject.OBB.Box, SecondObject.OBB.Location, SecondObject.Location, SecondObject.CurrentRotationMat,
							  FirstObject.Radius, FirstObject.Location, FirstObject.PrevLocation)) {
					SecondObject.Speed = 0.0f;
					CreateBulletExplosion(nullptr, SecondObject, -SecondObject.Num, SecondObject.Location, SecondObject.Speed);
					Command = eProjectilePairCycle::DeleteSecondObjectAndContinue;
				}
			}
		}
	});
}

} // astromenace namespace
} // viewizard namespace
