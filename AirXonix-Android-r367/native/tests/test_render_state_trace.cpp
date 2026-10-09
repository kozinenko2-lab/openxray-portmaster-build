#include "game/legacy_effects.hpp"
#include <cassert>
#include <iostream>
int main(){
    assert(kLegacyD3D7RenderState.alphaBlendWrapper==0x00405A60u);
    assert(kLegacyD3D7RenderState.alphaBlendEnableState==0x1B);
    assert(kLegacyD3D7RenderState.zFuncWrapper==0x00405A20u);
    assert(kLegacyD3D7RenderState.zFuncState==0x17);
    assert(kLegacyD3D7RenderState.zFuncLessEqual==4);
    assert(kLegacyD3D7RenderState.zFuncAlways==8);
    assert(kLegacyD3D7RenderState.textureHandleWrapper==0x004059F0u);
    assert(kLegacyTextureSlots.createRoutine==0x00405690u);
    assert(kLegacyTextureSlots.staticTableAddress==0x0043F040u);
    assert(kLegacyTextureSlots.handleTableAddress==0x004505B4u);
    assert(kLegacyTextureSlots.selectRoutine==0x004059F0u);
    assert(kLegacyTextureSlots.slotCount==9);
    assert(kLegacyTextureSlots.width[0]==64 && kLegacyTextureSlots.height[0]==64);
    assert(kLegacyTextureSlots.width[3]==256 && kLegacyTextureSlots.height[3]==256);
    assert(kLegacyTextureSlots.width[6]==128 && kLegacyTextureSlots.height[6]==128);
    assert(kLegacyAmbientBillboards.initAddress==0x00418200u);
    assert(kLegacyAmbientBillboards.renderAddress==0x00418290u);
    assert(kLegacyAmbientBillboards.deathRoutine==0x0041C030u);
    assert(kLegacyAmbientBillboards.initCallsite==0x0041C40Eu);
    assert(kLegacyAmbientBillboards.renderCallsite==0x0041CD61u);
    assert(kLegacyAmbientBillboards.inheritedTextureHandle==3);
    assert(!kLegacyAmbientBillboards.inheritedAlphaBlend);
    assert(kLegacyBillboardCallerOrder.billboardSetupCall==0x00414709u);
    assert(kLegacyBillboardCallerOrder.billboardSubmitCall==0x0041477Au);
    assert(kLegacyBillboardCallerOrder.alphaEnableCall==0x00414784u);
    assert(kLegacyBillboardCallerOrder.followingTextureHandle==6);
    assert(kLegacyBillboardCallerOrder.alphaDisableCall==0x00414797u);
    assert(kLegacyBillboardSubmitCallsites.size()==9);
    assert(kLegacyBillboardSubmitCallsites[0].submitCall==0x0041477Au);
    assert(kLegacyBillboardSubmitCallsites[1].baseAddress==0x0257E8D8u);
    assert(kLegacyBillboardSubmitCallsites[2].count==0x78);
    assert(kLegacyBillboardSubmitCallsites[3].dynamicCount);
    assert(kLegacyBillboardSubmitCallsites[4].baseAddress==0x0254CFE8u);
    assert(kLegacyBillboardSubmitCallsites[5].dynamicBase);
    assert(kLegacyBillboardSubmitCallsites[6].baseAddress==0x0254A2E8u);
    assert(kLegacyBillboardSubmitCallsites[7].baseAddress==0x0254B1E8u);
    assert(kLegacyBillboardSubmitCallsites[8].baseAddress==0x0254C0E8u);
    assert(kLegacyAmbientBillboards.totalRecords==480);
    assert(kLegacyAmbientBillboards.recordsPerGroup==160);
    assert(kLegacyDeathTriColorBillboards.groupBase[0]==0x0254A2E8u);
    assert(kLegacyDeathTriColorBillboards.groupBase[1]==0x0254B1E8u);
    assert(kLegacyDeathTriColorBillboards.groupBase[2]==0x0254C0E8u);
    assert(kLegacyDeathTriColorBillboards.groupBase[1]-kLegacyDeathTriColorBillboards.groupBase[0]==160u*24u);
    assert(kLegacyDeathTriColorBillboards.groupBase[2]-kLegacyDeathTriColorBillboards.groupBase[1]==160u*24u);
    assert(kLegacyAmbientBillboards.sourceXAddress==0x0257DA48u);
    assert(kLegacyAmbientBillboards.sourceYAddress==0x0257DA4Cu);
    assert(kLegacyAmbientBillboards.sourceZAddress==0x0257DA50u);
    assert(kLegacyAmbientBillboards.horizontalRandMask==0xFF);
    assert(kLegacyAmbientBillboards.horizontalRandBias==0x80);
    assert(kLegacyAmbientBillboards.verticalRandMask==0x7F);
    std::cout << "D3D7 caller-state + 480-record billboard trace ok\n";
}
