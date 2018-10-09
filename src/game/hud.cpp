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


	Website: https://viewizard.com/
	Project: https://github.com/viewizard/astromenace
	E-mail: viewizard@viewizard.com

*************************************************************************************/

// FIXME if we change game resolution/window size, we should also update particle systems position

// FIXME provide adaptive HUD size for high resolution, care about display dpi

// FIXME ostringstream is not so fast, re-use it only on real exp/money changes, but not all the time

// TODO translate comments

#include "../core/core.h"
#include "../config/config.h"
#include "../assets/texture.h"
#include "../object3d/space_ship/space_ship.h"
#include <sstream>
#include <iomanip>

// NOTE switch to nested namespace definition (namespace A::B::C { ... }) (since C++17)
namespace viewizard {
namespace astromenace {

namespace {

std::weak_ptr<cParticleSystem2D> EnergyEmblem{};
std::weak_ptr<cParticleSystem2D> ArmorEmblemCircle{};
std::weak_ptr<cParticleSystem2D> ArmorEmblemHoriz{};
std::weak_ptr<cParticleSystem2D> ArmorEmblemVert{};

float DrawBuffer[(2 + 2 + 4) * 6 * 16]; // RI_2f_XYZ | RI_2f_TEX | RI_4f_COLOR = (2 + 2 + 4) * 6 vertices * 16 characters
unsigned int DrawBufferCurrentPosition{0};

} // unnamed namespace

// FIXME should be fixed, don't allow global scope interaction for local variables
extern int GamePowerSystem;
extern float CurrentPlayerShipEnergy;
extern float CurrentAlert2;

float GetShipMaxEnergy(int Num);


/*
 * Init head-up display particle systems.
 * Make sure we re-init all particle systems data all the time in order to avoid
 * issues with future code changes.
 */
void InitHUDParticleSystems()
{
	if (EnergyEmblem.expired())
		EnergyEmblem = vw_CreateParticleSystem2D(vw_GetTimeThread(0));
	if (auto sharedEnergyEmblem = EnergyEmblem.lock()) {
		sharedEnergyEmblem->ColorStart = sRGBCOLOR{0.7f, 0.8f, 1.0f};
		sharedEnergyEmblem->ColorEnd = sRGBCOLOR{0.0f, 0.0f, 1.0f};
		sharedEnergyEmblem->AlphaStart = 1.0f;
		sharedEnergyEmblem->AlphaEnd = 1.0f;
		sharedEnergyEmblem->SizeStart = 12.0f;
		sharedEnergyEmblem->SizeVar = 10.0f;
		sharedEnergyEmblem->SizeEnd = 0.0f;
		sharedEnergyEmblem->Speed = 70.0f;
		sharedEnergyEmblem->SpeedVar = 20.0f;
		sharedEnergyEmblem->Theta = 360.0f;
		sharedEnergyEmblem->Life = 2.1f;
		sharedEnergyEmblem->LifeVar = 0.05f;
		sharedEnergyEmblem->ParticlesPerSec = 50;
		sharedEnergyEmblem->IsMagnet = true;
		sharedEnergyEmblem->MagnetFactor = 150.0f;
		sharedEnergyEmblem->CreationType = eParticle2DCreationType::Point;
		sharedEnergyEmblem->Texture = GetPreloadedTextureAsset("gfx/flare1.tga");
		sharedEnergyEmblem->MoveSystem(sVECTOR3D{33.0f, 29.0f, 0.0f});
	}

	if (ArmorEmblemVert.expired())
		ArmorEmblemVert = vw_CreateParticleSystem2D(vw_GetTimeThread(0));
	if (auto sharedArmorEmblemVert = ArmorEmblemVert.lock()) {
		sharedArmorEmblemVert->ColorStart = sRGBCOLOR{1.0f, 0.4f, 0.1f};
		sharedArmorEmblemVert->ColorEnd = sRGBCOLOR{0.5f, 0.0f, 0.0f};
		sharedArmorEmblemVert->AlphaStart = 1.0f;
		sharedArmorEmblemVert->AlphaEnd = 1.0f;
		sharedArmorEmblemVert->SizeStart = 13.0f;
		sharedArmorEmblemVert->SizeVar = 5.0f;
		sharedArmorEmblemVert->SizeEnd = 0.0f;
		sharedArmorEmblemVert->Speed = 0.0f;
		sharedArmorEmblemVert->SpeedOnCreation = 8.0f;
		sharedArmorEmblemVert->SpeedVar = 0.0f;
		sharedArmorEmblemVert->Theta = 360.0f;
		sharedArmorEmblemVert->Life = 2.0f;
		sharedArmorEmblemVert->LifeVar = 0.05f;
		sharedArmorEmblemVert->ParticlesPerSec = 50;
		sharedArmorEmblemVert->CreationType = eParticle2DCreationType::Quad;
		sharedArmorEmblemVert->CreationSize(1.0f, 18.0f, 0.0f);
		sharedArmorEmblemVert->Texture = GetPreloadedTextureAsset("gfx/flare1.tga");
		sharedArmorEmblemVert->MoveSystem(sVECTOR3D{GameConfig().InternalWidth - 33.0f, 29.0f, 0.0f});
	}

	if (ArmorEmblemHoriz.expired())
		ArmorEmblemHoriz = vw_CreateParticleSystem2D(vw_GetTimeThread(0));
	if (auto sharedArmorEmblemHoriz = ArmorEmblemHoriz.lock()) {
		sharedArmorEmblemHoriz->ColorStart = sRGBCOLOR{1.0f, 0.4f, 0.1f};
		sharedArmorEmblemHoriz->ColorEnd = sRGBCOLOR{0.5f, 0.0f, 0.0f};
		sharedArmorEmblemHoriz->AlphaStart = 1.0f;
		sharedArmorEmblemHoriz->AlphaEnd = 1.0f;
		sharedArmorEmblemHoriz->SizeStart = 13.0f;
		sharedArmorEmblemHoriz->SizeVar = 5.0f;
		sharedArmorEmblemHoriz->SizeEnd = 0.0f;
		sharedArmorEmblemHoriz->Speed = 0.0f;
		sharedArmorEmblemHoriz->SpeedOnCreation = 8.0f;
		sharedArmorEmblemHoriz->SpeedVar = 0.0f;
		sharedArmorEmblemHoriz->Theta = 360.0f;
		sharedArmorEmblemHoriz->Life = 2.0f;
		sharedArmorEmblemHoriz->LifeVar = 0.05f;
		sharedArmorEmblemHoriz->ParticlesPerSec = 50;
		sharedArmorEmblemHoriz->CreationType = eParticle2DCreationType::Quad;
		sharedArmorEmblemHoriz->CreationSize(18.0f, 1.0f, 0.0f);
		sharedArmorEmblemHoriz->Texture = GetPreloadedTextureAsset("gfx/flare1.tga");
		sharedArmorEmblemHoriz->MoveSystem(sVECTOR3D{GameConfig().InternalWidth - 33.0f, 29.0f, 0.0f});
	}

	if (ArmorEmblemCircle.expired())
		ArmorEmblemCircle = vw_CreateParticleSystem2D(vw_GetTimeThread(0));
	if (auto sharedArmorEmblemCircle = ArmorEmblemCircle.lock()) {
		sharedArmorEmblemCircle->ColorStart = sRGBCOLOR{1.0f, 0.6f, 0.2f};
		sharedArmorEmblemCircle->ColorEnd = sRGBCOLOR{0.5f, 0.0f, 0.0f};
		sharedArmorEmblemCircle->AlphaStart = 1.0f;
		sharedArmorEmblemCircle->AlphaEnd = 1.0f;
		sharedArmorEmblemCircle->SizeStart = 25.0f;
		sharedArmorEmblemCircle->SizeVar = 5.0f;
		sharedArmorEmblemCircle->SizeEnd = 0.0f;
		sharedArmorEmblemCircle->Speed = 0.0f;
		sharedArmorEmblemCircle->SpeedOnCreation = 8.0f;
		sharedArmorEmblemCircle->SpeedVar = 10.0f;
		sharedArmorEmblemCircle->Theta = 360.0f;
		sharedArmorEmblemCircle->Life = 1.5f;
		sharedArmorEmblemCircle->LifeVar = 0.05f;
		sharedArmorEmblemCircle->ParticlesPerSec = 70;
		sharedArmorEmblemCircle->Direction(1.0f, 0.0f, 0.0f);
		sharedArmorEmblemCircle->CreationType = eParticle2DCreationType::Circle;
		sharedArmorEmblemCircle->CreationSize(25.0f, 25.0f, 0.0f);
		sharedArmorEmblemCircle->DeadZone = 24.0f;
		sharedArmorEmblemCircle->IsMagnet = true;
		sharedArmorEmblemCircle->MagnetFactor = 25.0f;
		sharedArmorEmblemCircle->Texture = GetPreloadedTextureAsset("gfx/flare.tga");
		sharedArmorEmblemCircle->MoveSystem(sVECTOR3D{GameConfig().InternalWidth - 33.0f, 29.0f, 0.0f});
		sharedArmorEmblemCircle->SetRotation(sVECTOR3D{0.0f, 0.0f, 90.0f});
	}
}

/*
 * Update head-up display particle systems.
 */
void UpdateHUDParticleSystems(std::weak_ptr<cSpaceShip> &PlayerFighter)
{
	if (auto sharedPlayerFighter = PlayerFighter.lock()) {
		if (auto sharedEnergyEmblem = EnergyEmblem.lock()) {
			sharedEnergyEmblem->ParticlesPerSec =
				1 + static_cast<unsigned>(49 * (CurrentPlayerShipEnergy / GetShipMaxEnergy(GamePowerSystem)));
		}

		float tmpArmorPercentage = sharedPlayerFighter->ArmorCurrentStatus / sharedPlayerFighter->ArmorInitialStatus;
		bool tmpLowArmor = (sharedPlayerFighter->ArmorCurrentStatus < sharedPlayerFighter->ArmorInitialStatus / 10.0f);

		if (auto sharedArmorEmblemCircle = ArmorEmblemCircle.lock()) {
			sharedArmorEmblemCircle->ColorStart.r = 1.0f;
			sharedArmorEmblemCircle->ColorStart.g = 0.6f * tmpArmorPercentage;
			sharedArmorEmblemCircle->ColorStart.b = 0.2f * tmpArmorPercentage;

			if (tmpLowArmor) {
				sharedArmorEmblemCircle->AlphaStart = CurrentAlert2;
				sharedArmorEmblemCircle->AlphaEnd = CurrentAlert2;
			} else { // armor could be repaired in-game
				sharedArmorEmblemCircle->AlphaStart = 1.0f;
				sharedArmorEmblemCircle->AlphaEnd = 1.0f;
			}
		}

		if (auto sharedArmorEmblemHoriz = ArmorEmblemHoriz.lock()) {
			sharedArmorEmblemHoriz->ColorStart.r = 1.0f;
			sharedArmorEmblemHoriz->ColorStart.g = 0.6f * tmpArmorPercentage;
			sharedArmorEmblemHoriz->ColorStart.b = 0.2f * tmpArmorPercentage;

			if (tmpLowArmor) {
				if (CurrentAlert2 > 0.6f) {
					sharedArmorEmblemHoriz->AlphaStart = CurrentAlert2;
					sharedArmorEmblemHoriz->AlphaEnd = CurrentAlert2;
				} else { // armor could be repaired in-game
					sharedArmorEmblemHoriz->AlphaStart = 0.0f;
					sharedArmorEmblemHoriz->AlphaEnd = 0.0f;
				}
			} else {
				sharedArmorEmblemHoriz->AlphaStart = 1.0f;
				sharedArmorEmblemHoriz->AlphaEnd = 1.0f;
			}
		}

		if (auto sharedArmorEmblemVert = ArmorEmblemVert.lock()) {
			sharedArmorEmblemVert->ColorStart.r = 1.0f;
			sharedArmorEmblemVert->ColorStart.g = 0.6f * tmpArmorPercentage;
			sharedArmorEmblemVert->ColorStart.b = 0.2f * tmpArmorPercentage;

			if (tmpLowArmor) {
				if (CurrentAlert2 > 0.6f) {
					sharedArmorEmblemVert->AlphaStart = CurrentAlert2;
					sharedArmorEmblemVert->AlphaEnd = CurrentAlert2;
				} else { // armor could be repaired in-game
					sharedArmorEmblemVert->AlphaStart = 0.0f;
					sharedArmorEmblemVert->AlphaEnd = 0.0f;
				}
			} else {
				sharedArmorEmblemVert->AlphaStart = 1.0f;
				sharedArmorEmblemVert->AlphaEnd = 1.0f;
			}
		}
	} else {
		if (auto sharedEnergyEmblem = EnergyEmblem.lock()) {
			sharedEnergyEmblem->AlphaStart = 0.0f;
			sharedEnergyEmblem->AlphaEnd = 0.0f;
			sharedEnergyEmblem->ParticlesPerSec = 1;
		}

		if (auto sharedArmorEmblemCircle = ArmorEmblemCircle.lock()) {
			sharedArmorEmblemCircle->AlphaStart = 0.0f;
			sharedArmorEmblemCircle->AlphaEnd = 0.0f;
			sharedArmorEmblemCircle->ParticlesPerSec = 1;
		}

		if (auto sharedArmorEmblemHoriz = ArmorEmblemHoriz.lock()) {
			sharedArmorEmblemHoriz->AlphaStart = 0.0f;
			sharedArmorEmblemHoriz->AlphaEnd = 0.0f;
			sharedArmorEmblemHoriz->ParticlesPerSec = 1;
		}

		if (auto sharedArmorEmblemVert = ArmorEmblemVert.lock()) {
			sharedArmorEmblemVert->AlphaStart = 0.0f;
			sharedArmorEmblemVert->AlphaEnd = 0.0f;
			sharedArmorEmblemVert->ParticlesPerSec = 1;
		}
	}

	vw_UpdateAllParticleSystems2D(vw_GetTimeThread(0));
}

/*
 * Draw head-up display particle systems.
 */
void DrawHUDParticleSystems()
{
	vw_DrawAllParticleSystems2D();
}

/*
 * Draw head-up display border.
 */
void DrawHUDBorder()
{
	if (GameConfig().InternalWidth == 1024) {
		sRECT SrcRect{0, 0, 1024, 74};
		sRECT DstRect{0, 0, 1024, 74};
		vw_Draw2D(DstRect, SrcRect, GetPreloadedTextureAsset("game/game_panel.tga"), true, 1.0f);
		return;
	}

	sRECT SrcRect{0, 0, 466, 73};
	sRECT DstRect{0, 0, 466, 73};
	GLtexture tmpHUDBorder = GetPreloadedTextureAsset("game/game_panel2.tga");
	vw_Draw2D(DstRect, SrcRect, tmpHUDBorder, true, 1.0f);

	SrcRect(1, 74, 150, 145);
	DstRect(540, 0, 540 + 149, 71);
	vw_Draw2D(DstRect, SrcRect, tmpHUDBorder, true, 1.0f);

	SrcRect(150, 74, 610, 145);
	DstRect(768, 0, 768 + 460, 71);
	vw_Draw2D(DstRect, SrcRect, tmpHUDBorder, true, 1.0f);
}

/*
 * Sub-image rectangle in head-up display font texture for character.
 */
static void GetHUDCharacterRectangle(char Char, sRECT &Rect)
{
	switch (Char) {
	case '0':
		Rect(232, 4, 245, 25);
		break;
	case '1':
		Rect(71, 4, 84, 25);
		break;
	case '2':
		Rect(88, 4, 101, 25);
		break;
	case '3':
		Rect(106, 4, 119, 25);
		break;
	case '4':
		Rect(124, 4, 137, 25);
		break;
	case '5':
		Rect(142, 4, 155, 25);
		break;
	case '6':
		Rect(160, 4, 173, 25);
		break;
	case '7':
		Rect(178, 4, 191, 25);
		break;
	case '8':
		Rect(196, 4,209,25);
		break;
	case '9':
		Rect(214,4, 227, 25);
		break;

	case 'E': // star, experience symbol
		Rect(47, 4, 66, 25);
		break;
	case 'S': // second $ symbol, not in use
		Rect(4, 4, 21, 25);
		break;
	case '$':
		Rect(25, 4, 41 ,25);
		break;

	case ' ':
		Rect(0, 0, 13, 0);
		break;
	}
}

/*
 * Add data to local draw buffer.
 */
static inline void AddToDrawBuffer(float CoordX, float CoordY,
				   float Alpha, float TextureU, float TextureV)
{
	static sRGBCOLOR tmpColor{eRGBCOLOR::white};

	DrawBuffer[DrawBufferCurrentPosition++] = CoordX;
	DrawBuffer[DrawBufferCurrentPosition++] = CoordY;
	DrawBuffer[DrawBufferCurrentPosition++] = tmpColor.r;
	DrawBuffer[DrawBufferCurrentPosition++] = tmpColor.g;
	DrawBuffer[DrawBufferCurrentPosition++] = tmpColor.b;
	DrawBuffer[DrawBufferCurrentPosition++] = Alpha;
	DrawBuffer[DrawBufferCurrentPosition++] = TextureU;
	DrawBuffer[DrawBufferCurrentPosition++] = TextureV;
}

/*
 * Add character data to local draw buffer.
 * Return character width.
 */
static int AddCharToDrawBuffer(char Character, float Xstart, int Ystart,
			       float Alpha, float ImageWidth, float ImageHeight)
{
	sRECT SrcRect;
	GetHUDCharacterRectangle(Character, SrcRect);
	sRECT DstRect{static_cast<int>(Xstart), Ystart,
		      static_cast<int>(Xstart) + SrcRect.right - SrcRect.left, Ystart + SrcRect.bottom - SrcRect.top};

	// texture's UV coordinates
	float U_Left{SrcRect.left / ImageWidth};
	float V_Top{SrcRect.top / ImageHeight};
	float U_Right{SrcRect.right / ImageWidth};
	float V_Bottom{SrcRect.bottom / ImageHeight};

	// first triangle
	AddToDrawBuffer(DstRect.left, DstRect.top, Alpha, U_Left, V_Top);
	AddToDrawBuffer(DstRect.left, DstRect.bottom, Alpha, U_Left, V_Bottom);
	AddToDrawBuffer(DstRect.right, DstRect.bottom, Alpha, U_Right, V_Bottom);
	// second triangle
	AddToDrawBuffer(DstRect.right, DstRect.bottom, Alpha, U_Right, V_Bottom);
	AddToDrawBuffer(DstRect.right, DstRect.top, Alpha, U_Right, V_Top);
	AddToDrawBuffer(DstRect.left, DstRect.top, Alpha, U_Left, V_Top);

	return SrcRect.right - SrcRect.left;
}

/*
 * Add string data to local draw buffer.
 */
static void AddStringToDrawBuffer(const std::string &String, float Xstart, int Ystart,
				  float ImageWidth, float ImageHeight)
{
	// first '0' characters should be transparent for more nice look
	float Transp{0.2f};
	for (auto &tmpCharacter : String) {
		if (tmpCharacter != '0')
			Transp = 1.0f;

		Xstart += AddCharToDrawBuffer(tmpCharacter, Xstart, Ystart,
					      Transp, ImageWidth, ImageHeight);
	}
}

/*
 * Draw head-up display experience and money.
 */
void DrawHUDExpMoney(const int Experience, const int Money)
{
	GLtexture Texture = GetPreloadedTextureAsset("game/game_num.tga");
	if (!Texture)
		return;

	float ImageWidth{0.0f};
	float ImageHeight{0.0f};
	if (!vw_FindTextureSizeByID(Texture, &ImageWidth, &ImageHeight))
		return;

	DrawBufferCurrentPosition = 0;
	float Transp{1.0f};

	AddCharToDrawBuffer('E', GameConfig().InternalWidth / 2 - 57.0f, 5,
			    Transp, ImageWidth, ImageHeight);
	AddCharToDrawBuffer('$', GameConfig().InternalWidth / 2 - 56.0f, 31,
			    Transp, ImageWidth, ImageHeight);

	std::ostringstream tmpStream;
	tmpStream << std::fixed << std::setprecision(0)
		  << std::setfill('0') << std::setw(7)
		  << Experience;
	AddStringToDrawBuffer(tmpStream.str(),
			      GameConfig().InternalWidth / 2 - 57 + 23.0f, 5,
			      ImageWidth, ImageHeight);

	tmpStream.clear();
	tmpStream.str(std::string{});
	tmpStream << std::setfill('0') << std::setw(7)
		  << Money;
	AddStringToDrawBuffer(tmpStream.str(),
			      GameConfig().InternalWidth / 2 - 57 + 23.0f, 31,
			      ImageWidth, ImageHeight);

	vw_BindTexture(0, Texture);
	vw_SetTextureBlend(true, eTextureBlendFactor::SRC_ALPHA, eTextureBlendFactor::ONE_MINUS_SRC_ALPHA);

	vw_Draw3D(ePrimitiveType::TRIANGLES, 6 * 16, RI_2f_XY | RI_1_TEX | RI_4f_COLOR,
		  DrawBuffer, 8 * sizeof(DrawBuffer[0]));

	vw_SetTextureBlend(false, eTextureBlendFactor::ONE, eTextureBlendFactor::ZERO);
	vw_SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	vw_BindTexture(0, 0);
}

} // astromenace namespace
} // viewizard namespace
