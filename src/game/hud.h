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

#ifndef GAME_HUD_H
#define GAME_HUD_H

// NOTE switch to nested namespace definition (namespace A::B::C { ... }) (since C++17)
namespace viewizard {
namespace astromenace {

// Init HUD.
void InitHUD(std::weak_ptr<cSpaceShip> &SpaceShip, const int Experience, const int Money);
// Draw HUD.
void DrawHUD();
// Update HUD.
void UpdateHUD(std::weak_ptr<cSpaceShip> &SpaceShip);

// Update head-up display experience and money.
void UpdateHUDExpMoney(const int Experience, const int Money);


} // astromenace namespace
} // viewizard namespace

#endif // GAME_HUD_H
