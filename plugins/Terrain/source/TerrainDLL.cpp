//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "TerrainDLL.h"
#include <plugins/plugins_shared.h>

#include <sim/simObject.h>
#include <3d/rendering/common.h>
#include <graphics/utilities.h>

#include <bx/fpumath.h>

#include "TerrainBuilder.h"

using namespace Plugins;
PluginLink Plugins::Link;

bool                       terrainEnabled = false;
bgfx::ProgramHandle        terrainShader = BGFX_INVALID_HANDLE;
Rendering::RenderData*     terrainRenderData = NULL;
bgfx::VertexBufferHandle   terrainVB = BGFX_INVALID_HANDLE;
bgfx::IndexBufferHandle    terrainIB = BGFX_INVALID_HANDLE;
TerrainBuilder*            terrainBuilder = NULL;

// Called when the plugin is loaded.
void create(PluginLink _link)
{
   Link = _link;

   // Load Shader
   Graphics::ShaderAsset* terrainShaderAsset = Link.Graphics.getShaderAsset("Terrain:terrainShader");
   if ( terrainShaderAsset )
      terrainShader = terrainShaderAsset->getProgram();

   // Register Console Functions
   Link.Con.addCommand("Terrain", "load", loadHeightMap, "", 2, 2);
   Link.Con.addCommand("Terrain", "enable", enableTerrain, "", 1, 1);
   Link.Con.addCommand("Terrain", "disable", disableTerrain, "", 1, 1);

   // Create a terrain builder
   terrainBuilder = new TerrainBuilder(_link);
}

void destroy()
{
   SAFE_DELETE(terrainBuilder);

   if ( terrainVB.idx != bgfx::invalidHandle )
      Link.bgfx.destroyVertexBuffer(terrainVB);

   if ( terrainIB.idx != bgfx::invalidHandle )
      Link.bgfx.destroyIndexBuffer(terrainIB);
}

// Console Functions
void loadHeightMap(SimObject *obj, S32 argc, const char *argv[])
{
   GBitmap *bmp = dynamic_cast<GBitmap*>(Link.ResourceManager->loadInstance(argv[1]));   
   if(bmp != NULL)
   {
      terrainBuilder->height = (bmp->getHeight() / 2) * 2;
      terrainBuilder->width = (bmp->getWidth() / 2) * 2;
      terrainBuilder->heightMap = new F32[terrainBuilder->height * terrainBuilder->width];

      for(U32 y = 0; y < terrainBuilder->height; y++)
      {
         for(U32 x = 0; x < terrainBuilder->width; x++)
         {
            ColorI heightSample;
            bmp->getColor(x, y, heightSample);
            terrainBuilder->heightMap[(y * terrainBuilder->width) + x] = ((F32)heightSample.red) * 0.1f;
         }
      }

      terrainBuilder->rebuild();
      refresh();
   }
}

void refresh()
{
   if ( terrainRenderData == NULL )
      terrainRenderData = Link.Rendering.createRenderData();

   // Destroy Old Buffers
   if ( terrainVB.idx != bgfx::invalidHandle )
      Link.bgfx.destroyVertexBuffer(terrainVB);

   if ( terrainIB.idx != bgfx::invalidHandle )
      Link.bgfx.destroyIndexBuffer(terrainIB);

   // Get New Ones
   terrainVB = terrainBuilder->getVertexBuffer();
	terrainIB = terrainBuilder->getIndexBuffer();
   
   terrainRenderData->indexBuffer = terrainIB;
   terrainRenderData->vertexBuffer = terrainVB;

   // Render in Forward (for now) with our custom terrain shader.
   terrainRenderData->shader = terrainShader;
   terrainRenderData->view = Graphics::ViewTable::Forward;

   // Transform
   F32* cubeMtx = new F32[16];
   bx::mtxSRT(cubeMtx, 1, 1, 1, 0, 0, 0, 0, 0, 0);
   terrainRenderData->transformTable = cubeMtx;
   terrainRenderData->transformCount = 1;
}

void enableTerrain(SimObject *obj, S32 argc, const char *argv[])
{
   terrainEnabled = true;
}

void disableTerrain(SimObject *obj, S32 argc, const char *argv[])
{
   terrainEnabled = false;
}