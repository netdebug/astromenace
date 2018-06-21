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


	Web Site: https://www.viewizard.com/
	Project: https://github.com/viewizard/astromenace
	E-mail: viewizard@viewizard.com

*************************************************************************************/

#ifndef ASSETS_TEXTURE_H
#define ASSETS_TEXTURE_H

#include "../core/base.h"

// Get all texture assets value.
unsigned GetTextureAssetsValue();
// Cycle with function call on each texture asset load.
void ForEachTextureAssetLoad(std::function<void (unsigned AssetValue)> function);
// Change anisotropy level for all textures that was loaded with anisotropy.
void ChangeTexturesAnisotropyLevel();

#endif // ASSETS_TEXTURE_H