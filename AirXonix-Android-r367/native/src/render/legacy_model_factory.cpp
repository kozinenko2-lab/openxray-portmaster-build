#include "legacy_model_factory.hpp"
#include "legacy_mesh_builder.hpp"
#include "legacy_transform.hpp"
#include "legacy_models.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace LegacyModelFactory {
namespace {
constexpr float kPi=3.14159265358979323846f;
constexpr float kQuarterPi=0.78539816339744830962f;
constexpr float kHalfPi=1.57079632679489661923f;
constexpr float kChildRadius=0.003f;

LegacyMesh buildGroundSpike(bool highQuality) {
    LegacyMeshBuilder b;
    // Local profile at EBP-78 in 0x420C10:
    // { 0.01, 1.25 }, { 0.315, 0.0 }
    const std::vector<LegacyProfilePoint> profile={{0.01f,1.25f},{0.315f,0.f}};
    b.addRevolvedProfile(profile,
                         kPi/6.f,kPi/6.f,
                         highQuality ? 4 : 3,
                         0.2265625f,0.2265625f,
                         0.001953125f,0.029296875f,
                         0.002f);
    return b.take();
}

void appendLatitudeRing(LegacyMeshBuilder& body,const LegacyMesh& spike,
                        int count,int zRotationUnits,float latitude) {
    const float radial=std::cos(latitude)*kChildRadius;
    const float y=std::sin(latitude)*kChildRadius;
    for (int i=0;i<count;++i) {
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        LegacyTransform::rotateZ(m,zRotationUnits);
        // Exact integer-angle progression observed in 0x420D2A..0x420F0D.
        LegacyTransform::rotateY(m,(2048/count)*i);
        const float a=2.f*kPi*float(i)/float(count);
        body.appendMeshTransformed(spike,m,
                                   std::cos(a)*radial,
                                   y,
                                   std::sin(a)*radial);
    }
}
}

LegacyMesh buildGroundEnemy(bool highQuality) {
    const LegacyMesh spike=buildGroundSpike(highQuality);
    LegacyMeshBuilder body;

    // 0x420CF3: 12-segment adaptive rounded body.
    body.addAdaptiveSphere(12,0.0033f,
                           0.189453125f,0.216796875f,
                           0.001953125f,0.029296875f);

    // 0x420CF8..0x420D00: unrotated north-pole spike.
    body.appendMesh(spike,0.f,0.003f,0.f);

    // 0x420D2A..0x420DAF: four spikes at +45 degrees latitude.
    appendLatitudeRing(body,spike,4,-256,+kQuarterPi);
    // 0x420DD9..0x420E5E: eight equatorial spikes.
    appendLatitudeRing(body,spike,8,-512,0.f);
    // 0x420E88..0x420F0D: four spikes at -45 degrees latitude.
    appendLatitudeRing(body,spike,4,-768,-kQuarterPi);

    return body.take();
}

LegacyMesh buildAirEnemySubtype(int subtype,bool highQuality) {
    subtype=std::clamp(subtype,0,3);
    static constexpr float kU0[4]={0.001953125f,0.001953125f,0.064453125f,0.064453125f};
    static constexpr float kU1[4]={0.060546875f,0.060546875f,0.123046875f,0.123046875f};
    static constexpr float kV0[4]={0.001953125f,0.064453125f,0.001953125f,0.064453125f};
    static constexpr float kV1[4]={0.060546875f,0.123046875f,0.060546875f,0.123046875f};

    LegacyMeshBuilder b;
    if (!highQuality) {
        // 0x421685..0x4216EB: legacy low-detail path uses a simple sphere.
        b.addAdaptiveSphere(16,0.0048f,kU0[subtype],kU1[subtype],kV0[subtype],kV1[subtype]);
        return b.take();
    }

    // 0x421546..0x421579 constructs eight profile points directly on the
    // stack. 0x43B5E0=0.44308468699 and 0x43B300=0.02.
    constexpr float step=0.4430846869945526f;
    constexpr float bias=0.019999999552965164f;
    std::vector<LegacyProfilePoint> profile;
    profile.reserve(8);
    for (int i=0;i<8;++i) {
        const float a=kHalfPi-float(i)*step-bias;
        profile.push_back({std::cos(a),std::sin(a)});
    }
    b.addRevolvedProfile(profile,
                         +kHalfPi,-kHalfPi,
                         16,
                         kU0[subtype],kU1[subtype],
                         kV0[subtype],kV1[subtype],
                         0.005f);
    return b.take();
}


LegacyMesh buildAirEnemyShadow() {
    // r47 DIRECT EXE: 0x422D20 emits exactly fourteen 24-byte ring vertices
    // and six 4-index face records.  0x422D3D..0x422D6A stores transformed-
    // workspace byte offsets with a 44-byte stride; dividing those offsets by
    // 44 yields the exact quads below.  There is no center vertex/fan.
    LegacyMeshBuilder b;
    constexpr int segments=LegacyModels::AirEnemyShadow.segments;
    constexpr float radius=LegacyModels::AirEnemyShadow.radius;
    constexpr float step=LegacyModels::AirEnemyShadow.angleStep;
    std::array<std::uint32_t,segments> ring{};
    for(int i=0;i<segments;++i){
        const float a=step*static_cast<float>(i);
        LegacyMasterVertex v{};
        v.y=LegacyModels::AirEnemyShadow.builderY;
        v.nx=0.f; v.ny=1.f; v.nz=0.f;
        v.x=std::cos(a)*radius;
        v.z=std::sin(a)*radius;
        ring[static_cast<std::size_t>(i)]=b.addVertex(v);
    }
    constexpr std::array<std::array<int,4>,6> faces{{
        {{1,0,13,12}}, {{2,1,12,11}}, {{3,2,11,10}},
        {{4,3,10,9}}, {{5,4,9,8}}, {{6,5,8,7}}
    }};
    for(const auto& f:faces){
        b.addQuad(ring[static_cast<std::size_t>(f[0])],
                  ring[static_cast<std::size_t>(f[1])],
                  ring[static_cast<std::size_t>(f[2])],
                  ring[static_cast<std::size_t>(f[3])]);
    }
    return b.take();
}

XonixModelParts buildXonix(bool highQuality) {
    XonixModelParts out{};

    // 0x42066B..0x4206A6: reusable upper dome/cap from 0x4020D0.
    LegacyMeshBuilder domeBuilder;
    domeBuilder.addAdaptiveHemisphere(highQuality ? 16 : 12,
                                      0.005f,
                                      0.126953125f,0.154296875f,
                                      0.001953125f,0.029296875f);
    const LegacyMesh dome=domeBuilder.take();

    LegacyMeshBuilder body;
    // 0x4206C8..0x420719: central lathed profile.
    body.addRevolvedProfile({{0.01f,3.0f},{0.2f,2.9f},{0.2f,2.5f}},
                            kQuarterPi,0.f,
                            highQuality ? 8 : 6,
                            0.126953125f,0.154296875f,
                            0.033203125f,0.060546875f,
                            0.005f);
    // First dome is upright at y=.0075.
    body.appendMesh(dome,0.f,0.0075f,0.f);
    // 0x4011D0 is an X-axis rotation on a finalized mesh.  The x86 builder
    // flips the second dome by pi before appending it at y=.00505.
    LegacyTransform::Matrix34 flip=LegacyTransform::identity();
    LegacyTransform::rotateX(flip,1024); // pi in 0..2047 legacy angle units.
    body.appendMeshTransformed(dome,flip,0.f,0.00505f,0.f);

    // 0x420768..0x4207A5: cylindrical neck/ring.
    body.addRevolvedProfile({{0.7f,1.5f},{0.7f,1.0f}},
                            0.f,0.f,
                            highQuality ? 16 : 12,
                            0.158203125f,0.185546875f,
                            0.001953125f,0.029296875f,
                            0.005f);
    // 0x4207C8..0x420811: horizontal lip/disc edge.
    body.addRevolvedProfile({{0.7f,1.0f},{1.02f,1.0f}},
                            kHalfPi,kHalfPi,
                            highQuality ? 16 : 12,
                            0.158203125f,0.185546875f,
                            0.001953125f,0.029296875f,
                            0.005f);
    out.body=body.take();

    // 0x42085C..0x42091D: two opposite 30-degree radial sectors at y=.014.
    // This is the propeller/fan mesh drawn once at Xonix's center.
    LegacyMeshBuilder prop;
    const std::vector<LegacyProfilePoint> blade={{0.2f,2.8f},{1.6f,2.8f}};
    const int bladeSegments=highQuality ? 3 : 2;
    prop.addRevolvedProfileArc(0.f,kPi/6.f,blade,kHalfPi,kHalfPi,bladeSegments,
                               0.126953125f,0.154296875f,
                               0.033203125f,0.060546875f,0.005f);
    prop.addRevolvedProfileArc(kPi,7.f*kPi/6.f,blade,kHalfPi,kHalfPi,bladeSegments,
                               0.126953125f,0.154296875f,
                               0.033203125f,0.060546875f,0.005f);
    out.propeller=prop.take();

    // 0x420982..0x4209D0: one sphere reused four times by 0x4209E0.
    LegacyMeshBuilder node;
    node.addAdaptiveSphere(highQuality ? 12 : 8,
                           0.0018f,
                           0.2421875f,0.2421875f,
                           0.0390625f,0.0390625f);
    out.rotorNode=node.take();
    return out;
}

}

namespace LegacyModelFactory {
namespace {
LegacyMesh buildPickupCommonA(bool /*highQuality*/) {
    LegacyMeshBuilder b;
    // 0x421739..0x421793: reusable two-point pointed/needle submesh.
    b.addRevolvedProfile({{0.0003125f,0.00425f},{0.00125f,0.f}},
                         0.f,0.f,8,
                         0.19921875f,0.19921875f,
                         0.05859375f,0.05859375f,
                         1.f);
    return b.take();
}

LegacyMesh buildPickupCommonB(bool highQuality) {
    LegacyMeshBuilder b;
    // 0x4217A3..0x421824. The high-quality switch changes only radial tessellation.
    b.addRevolvedProfile({{0.0008333333f,0.0075f},{0.0008333333f,-0.000045f}},
                         0.f,0.f,highQuality ? 8 : 4,
                         0.22265625f,0.22265625f,
                         0.05859375f,0.05859375f,
                         1.f);
    return b.take();
}

LegacyMesh buildPickupCommonC(bool highQuality) {
    LegacyMeshBuilder b;
    // 0x421829..0x4218FC: reusable partial annular solid used by pickup 5.
    const std::vector<LegacyProfilePoint> profile={
        {0.003f,+0.0005f},{0.0045f,+0.0005f},{0.0045f,-0.0005f},
        {0.003f,-0.0005f},{0.003f,+0.0005f}
    };
    b.addRevolvedProfileArc(-2.5132741928f,1.5707963705f,profile,
                            2.3561944962f,2.3561944962f,
                            highQuality ? 16 : 6,
                            0.234375f,0.234375f,
                            0.05859375f,0.05859375f,
                            1.f);
    return b.take();
}

LegacyMesh buildPickupCommonD(bool highQuality) {
    LegacyMeshBuilder b;
    // 0x421902..0x42199F: second reusable three-point revolved profile.
    // Direct xrefs from pickup type 5 at 0x42204A and 0x42207A prove that
    // this model ([ebp-0xF4]/ESI) is used for the first two components of
    // the question-mark pickup; only the third component uses common C.
    b.addRevolvedProfile({
            {0.0000625f,0.0021f},
            {0.000625f, 0.0020f},
            {0.0016666666f,0.f}},
            0.f,0.f,highQuality ? 8 : 5,
            0.234375f,0.234375f,
            0.05859375f,0.05859375f,
            1.f);
    return b.take();
}
}

LegacyMesh buildPickupType(int type,bool highQuality) {
    type=std::clamp(type,0,5);
    LegacyMeshBuilder b;

    if (type==0) {
        // 0x4219B6..0x421AD9: score/+1000 pickup. A thin cylinder/coin whose
        // circular faces map exactly to the 32x32 MONY atlas tile centered at
        // pixel (48,48) in atlas 3 (u=v=0.1875, radius about 15.5 px).
        const int seg=highQuality ? 22 : 12;
        b.addRevolvedProfile({{0.005f,+0.000625f},{0.005f,-0.000625f}},
                             0.f,0.f,seg,
                             0.126953125f,0.126953125f,
                             0.126953125f,0.126953125f,
                             1.f);
        b.addCircularCap(0.f,+0.000625f,0.f,0.005f,seg,
                         0.1875f,0.1875f,highQuality ? 0.059765625f : 0.060546875f,false);
        b.addCircularCap(0.f,-0.000625f,0.f,0.005f,seg,
                         0.1875f,0.1875f,highQuality ? 0.059765625f : 0.060546875f,true);
        return b.take();
    }

    if (type==1) {
        // 0x421C1E..0x421CF8. Two differently rotated copies of common A plus
        // a closed lathed ring. 4011D0 rotates a finalized mesh about X;
        // 401240 rotates it about Z. 401490 always transforms source->scratch,
        // so the two placements are independent rather than cumulative.
        const LegacyMesh needle=buildPickupCommonA(highQuality);
        LegacyTransform::Matrix34 mx=LegacyTransform::identity();
        LegacyTransform::rotateX(mx,-512); // -pi/2
        b.appendMeshTransformed(needle,mx,0.f,0.f,0.f);
        LegacyTransform::Matrix34 mz=LegacyTransform::identity();
        LegacyTransform::rotateZ(mz,+512); // +pi/2
        b.appendMeshTransformed(needle,mz,0.f,0.f,0.f);
        b.addRevolvedProfile({
                {0.0045f,+0.00075f},{0.0055f,+0.00075f},{0.0055f,-0.00075f},
                {0.0045f,-0.00075f},{0.0045f,+0.00075f}},
                2.3561944962f,2.3561944962f,
                highQuality ? 20 : 10,
                0.189453125f,0.236328125f,
                0.03515625f,0.03515625f,
                1.f);
        return b.take();
    }

    if (type==2) {
        // 0x421D39..0x421E86: exact 18-point heart contour used by the +1 life
        // pickup. The first and last points are intentionally identical.
        const std::vector<LegacyProfilePoint> heart={
            { 0.0f,  3.0f}, { 2.0f,  5.0f}, { 3.0f,  5.3f}, { 4.0f,  5.0f},
            { 4.7f,  4.0f}, { 5.0f,  3.0f}, { 4.8f,  2.0f}, { 4.0f,  0.0f},
            { 0.1f, -5.0f}, {-0.1f, -5.0f}, {-4.0f,  0.0f}, {-4.8f,  2.0f},
            {-5.0f,  3.0f}, {-4.7f,  4.0f}, {-4.0f,  5.0f}, {-3.0f,  5.3f},
            {-2.0f,  5.0f}, { 0.0f,  3.0f}
        };
        b.addExtrudedContour(heart,
                             2.3561944961547852f,0.7853981852531433f,
                            -1.0f,+1.0f,
                             0.2421875f,0.0546875f,
                             0.0011f);
        return b.take();
    }

    if (type==3) {
        // 0x421AE7..0x421C10: warning/speed-effect disc. UV center (16,48)
        // lands exactly at the center of the SPEE 32x32 atlas tile (x=0,y=32).
        const int seg=highQuality ? 18 : 10;
        b.addRevolvedProfile({{0.005f,+0.000625f},{0.005f,-0.000625f}},
                             0.f,0.f,seg,
                             0.001953125f,0.001953125f,
                             0.126953125f,0.126953125f,
                             1.f);
        if(highQuality){
            // DIRECT EXE 0x421B45..0x421B9B: type 3/SPEE uses the
            // 0x403400/0x4036E0 high-quality pair with this literal normal
            // tilt. UV center/radius are unchanged; only the cap lighting was
            // flattened incorrectly by the old native triangle-fan path.
            constexpr float tilt=1.3744468688964844f;
            b.addBeveledCircularCap(0.f,+0.000625f,0.f,0.005f,seg,
                                    0.0625f,0.1875f,0.060546875f,tilt,false);
            b.addBeveledCircularCap(0.f,-0.000625f,0.f,0.005f,seg,
                                    0.0625f,0.1875f,0.060546875f,tilt,true);
        }else{
            b.addCircularCap(0.f,+0.000625f,0.f,0.005f,seg,
                             0.0625f,0.1875f,0.060546875f,false);
            b.addCircularCap(0.f,-0.000625f,0.f,0.005f,seg,
                             0.0625f,0.1875f,0.060546875f,true);
        }
        return b.take();
    }

    if (type==4) {
        // 0x421E9C..0x422013. Three independently transformed copies of the
        // reusable long rod plus a thin closed lathed ring.
        const LegacyMesh rod=buildPickupCommonB(highQuality);
        LegacyTransform::Matrix34 m1=LegacyTransform::identity();
        LegacyTransform::rotateX(m1,-512);
        LegacyTransform::rotateY(m1,0x34c);
        b.appendMeshTransformed(rod,m1,-0.0002f,0.f,-0.00415f);

        LegacyTransform::Matrix34 m2=m1;
        LegacyTransform::rotateY(m2,0x168);
        b.appendMeshTransformed(rod,m2,+0.0002f,0.f,-0.00415f);

        LegacyTransform::Matrix34 m3=LegacyTransform::identity();
        LegacyTransform::rotateZ(m3,-512);
        LegacyTransform::setScale(m3,0.7f);
        b.appendMeshTransformed(rod,m3,-0.00235f,0.f,+0.0005f);

        b.addRevolvedProfile({
                {0.0045f,+0.0005f},{0.0055f,+0.0005f},{0.0055f,-0.0005f},
                {0.0045f,-0.0005f},{0.0045f,+0.0005f}},
                2.3561944962f,2.3561944962f,
                highQuality ? 20 : 10,
                0.22265625f,0.22265625f,
                0.046875f,0.046875f,
                1.f);
        return b.take();
    }

    if (type==5) {
        // 0x422021..0x4220C5. Two independent placements of common C and one
        // placement of the second prebuilt partial arc. 401490 transforms from
        // the immutable source vertices each time, which matters here.
        // Direct xrefs: the first two transformed components use ESI, which
        // is restored from [ebp-0xF4] at 0x4219A3. The third explicitly reloads
        // [ebp-0xF8] at 0x4220A6. This is the question-mark recipe.
        const LegacyMesh terminal=buildPickupCommonD(highQuality);
        const LegacyMesh hook=buildPickupCommonC(highQuality);

        LegacyTransform::Matrix34 m1=LegacyTransform::identity();
        LegacyTransform::rotateX(m1,-512);
        LegacyTransform::setScale(m1,0.8f);
        b.appendMeshTransformed(terminal,m1,0.f,0.f,+0.00145f);

        LegacyTransform::Matrix34 m2=LegacyTransform::identity();
        LegacyTransform::rotateX(m2,-512);
        b.appendMeshTransformed(terminal,m2,0.f,0.f,+0.006f);

        LegacyTransform::Matrix34 identity=LegacyTransform::identity();
        b.appendMeshTransformed(hook,identity,0.f,0.f,-0.0035f);
        return b.take();
    }

    // All six normal gameplay pickup branches are now represented by native
    // procedural recipes. This fallback is unreachable after type clamping.
    return b.take();
}
LegacyMesh buildTrailSegment(bool damaged,bool highQuality) {
    LegacyMeshBuilder b;
    if (!damaged) {
        // 0x4213A6..0x421430. 3-point profile, pi/4 -> 0 endpoint normals.
        b.addRevolvedProfile({{0.01f,1.0f},{0.25f,0.7f},{0.25f,0.0f}},
                             kQuarterPi,0.f,
                             highQuality ? 6 : 4,
                             0.158203125f,0.162109375f,
                             0.033203125f,0.060546875f,
                             0.006f);
    } else {
        // 0x42147D..0x421507. Damaged/hazard trail segment.
        b.addRevolvedProfile({{0.01f,1.5f},{0.35f,1.2f},{0.35f,0.0f}},
                             kQuarterPi,0.f,
                             highQuality ? 6 : 4,
                             0.181640625f,0.185546875f,
                             0.033203125f,0.060546875f,
                             0.0055f);
    }
    return b.take();
}

HomingModelParts buildHoming(bool highQuality) {
    HomingModelParts out{};

    // 0x420F47..0x420FC3: small two-point lathe used as one radial tooth.
    LegacyMeshBuilder toothBuilder;
    toothBuilder.addRevolvedProfile({{0.01f,1.0f},{0.3f,0.0f}},
                                    0.5235987902f,0.5235987902f,
                                    highQuality ? 4 : 3,
                                    0.2265625f,0.2265625f,
                                    0.001953125f,0.029296875f,
                                    0.003f);
    const LegacyMesh tooth=toothBuilder.take();

    // 0x420FCD..0x421052: seven-point curved shell profile generated at
    // runtime from the x87 trigonometric constants. The builder writes eight
    // points but passes pointCount=7 to 0x402A50; preserve that exact quirk.
    constexpr float kStep=0.39269909262657166f;      // pi/8, 0x43B464
    constexpr float kStart=1.5707963705062866f;      // pi/2, 0x43B26C
    constexpr float kRadiusBias=0.012271846644580364f; // pi/256, 0x43B5DC
    constexpr float kHeightBias=0.04908738657832146f; // pi/64, 0x43B5B8
    std::vector<LegacyProfilePoint> shellProfile;
    shellProfile.reserve(7);
    for(int i=0;i<7;++i){
        const float theta=kStart-float(i)*kStep;
        shellProfile.push_back({2.f*std::cos(theta-kRadiusBias),
                                std::sin(theta-kHeightBias)});
    }

    LegacyMeshBuilder body;
    body.addRevolvedProfile(shellProfile,
                            +1.5707963705f,-1.5707963705f,
                            highQuality ? 16 : 10,
                            0.189453125f,0.216796875f,
                            0.064453125f,0.091796875f,
                            0.003f);

    // 0x42107C..0x421101: eight independently transformed copies of the
    // tooth around a horizontal ring of radius .0056. Every tooth is first
    // turned by -pi/2 about Z, then by i*pi/4 about Y.
    constexpr float kRingRadius=0.0055999998f;
    for(int i=0;i<8;++i){
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        LegacyTransform::rotateZ(m,-512);
        LegacyTransform::rotateY(m,i*256);
        const float a=float(i)*0.7853981852531433f;
        // r194 DIRECT EXE 0x4210BE..0x4210E6: 0x403160(mesh,x,y,z) receives
        // x=cos(a)*cos(c)*.0056, y=sin(c)*.0028, z=sin(a)*cos(c)*.0056 with
        // c=[0x43B5C8]=0.0. r4x..r193 swapped sin/cos, so the teeth sat 90
        // degrees away from their own -Z/+Y orientation and stuck out sideways.
        body.appendMeshTransformed(tooth,m,
                                   std::cos(a)*kRingRadius,
                                   0.f,
                                   std::sin(a)*kRingRadius);
    }
    out.body=body.take();

    // 0x421124..0x421179: separately finalized adaptive hemisphere. The
    // runtime draws it at bodyY + .0017 + cos(soundPhase)*.001.
    LegacyMeshBuilder dome;
    dome.addAdaptiveHemisphere(highQuality ? 16 : 12,
                               0.002f,
                               0.12890625f,0.12890625f,
                               0.06640625f,0.06640625f);
    out.dome=dome.take();
    return out;
}

EraserModelParts buildEraser(bool highQuality) {
    EraserModelParts out{};
    // 0x421199..0x421285: a two-point lathe copied eight times around the Y
    // axis after a +pi/2 X rotation, forming the eraser's radial star.
    LegacyMeshBuilder needleBuilder;
    needleBuilder.addRevolvedProfile({{0.01f,2.0f},{0.3f,0.0f}},
                                     0.5235987902f,0.5235987902f,
                                     highQuality ? 6 : 4,
                                     0.24609375f,0.24609375f,
                                     0.001953125f,0.029296875f,
                                     0.003f);
    const LegacyMesh needle=needleBuilder.take();
    LegacyMeshBuilder star;
    for(int i=0;i<8;++i){
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        LegacyTransform::rotateX(m,512);
        LegacyTransform::rotateY(m,i*256);
        star.appendMeshTransformed(needle,m);
    }
    out.star=star.take();

    // 0x4212A1..0x42137E: the second 9-point lathed component. The profile is
    // copied literally from the stack constants written by the x86 builder.
    LegacyMeshBuilder core;
    core.addRevolvedProfile({
            {0.01f,1.5f},{1.0f,1.0f},{1.0f,0.99f},
            {1.0f,0.5f},{0.99f,0.49f},{0.2f,0.0f},
            {1.0f,-0.49f},{1.0f,-0.5f},{0.7f,-1.0f}},
            kHalfPi,-kQuarterPi,
            highQuality ? 16 : 10,
            0.189453125f,0.216796875f,
            0.095703125f,0.123046875f,
            0.004f);
    out.core=core.take();
    return out;
}


LegacyMesh buildInformationFlyingMine() {
    // DIRECT EXE 0x422CBB..0x422D0C -> 0x404000. This is the model used
    // by 0x414379/0x41438B on the additional-enemies Information page.
    // It is a single horizontal quad, NOT airborne-enemy subtype 3.
    // 0x404000 signature: x,y,z,sizeX,sizeZ,u0,u1,v0,v1.
    LegacyMeshBuilder b;
    constexpr float x=-0.8f, y=0.f, z=-0.8f, sx=1.6f, sz=1.6f;
    constexpr float u0=0.f,u1=4.f,v0=0.f,v1=4.f;
    LegacyMasterVertex a{}; a.x=x;    a.y=y; a.z=z;    a.ny=1.f; a.u=u0; a.v=v1;
    LegacyMasterVertex c{}; c.x=x;    c.y=y; c.z=z+sz; c.ny=1.f; c.u=u0; c.v=v0;
    LegacyMasterVertex d{}; d.x=x+sx; d.y=y; d.z=z+sz; d.ny=1.f; d.u=u1; d.v=v0;
    LegacyMasterVertex e{}; e.x=x+sx; e.y=y; e.z=z;    e.ny=1.f; e.u=u1; e.v=v1;
    const auto ia=b.addVertex(a), ic=b.addVertex(c), id=b.addVertex(d), ie=b.addVertex(e);
    b.addQuad(ia,ic,id,ie);
    return b.take();
}


LegacyMesh buildAuxiliarySlot(int slot) {
    LegacyMeshBuilder b;
    // Literal 0x422248..0x422396 -> 0x4041A0 argument sets.
    switch(slot){
    case 0: b.addBeveledPlane(-0.007499999832361937f,0.f,-0.003000000026077032f,
                              0.014999999664723873f,0.006000000052154064f,0.0010000000474974513f,
                              0.001953125f,0.498046875f,0.00390625f,0.18359375f); break;
    case 1: b.addBeveledPlane(-0.007499999832361937f,0.f,-0.003000000026077032f,
                              0.014999999664723873f,0.006000000052154064f,0.0010000000474974513f,
                              0.501953125f,0.998046875f,0.00390625f,0.18359375f); break;
    case 2: b.addBeveledPlane(-0.007499999832361937f,0.f,-0.003000000026077032f,
                              0.014999999664723873f,0.006000000052154064f,0.0010000000474974513f,
                              0.001953125f,0.498046875f,0.19140625f,0.37109375f); break;
    case 3: b.addBeveledPlane(-0.007499999832361937f,0.f,-0.003000000026077032f,
                              0.014999999664723873f,0.006000000052154064f,0.0010000000474974513f,
                              0.501953125f,0.998046875f,0.19140625f,0.37109375f); break;
    case 4: b.addBeveledPlane(-0.007499999832361937f,0.f,-0.003000000026077032f,
                              0.014999999664723873f,0.006000000052154064f,0.0010000000474974513f,
                              0.001953125f,0.498046875f,0.37890625f,0.55859375f); break;
    case 5: b.addBeveledPlane(-0.019999999552965164f,0.f,-0.0035000001080334187f,
                              0.03999999910593033f,0.007000000216066837f,0.0010000000474974513f,
                              0.501953125f,0.998046875f,0.564453125f,0.748046875f); break;
    default: break;
    }
    return b.take();
}


LegacyMesh buildCinematicSlot(int slot) {
    LegacyMeshBuilder b;
    switch(slot){
    case 0:
        // 0x4223F1..0x422423 -> 0x404620. Argument order after reversing
        // x86 pushes: minX, halfHeight, minZ, sizeX, sizeZ, bevel, u0,u1,v0,v1.
        b.addBeveledPrism(-0.014999999664723873f,0.00039999998989515007f,
                          -0.003000000026077032f,0.029999999329447746f,
                          0.006000000052154064f,0.0010000000474974513f,
                          0.001953125f,0.998046875f,0.19921875f,0.373046875f);
        break;
    case 1:
        // 0x42242D..0x422466 -> 0x404620. LEV2 / "Stage:" plaque.
        b.addBeveledPrism(-0.007000000216066837f,0.f,-0.002199999988079071f,
                          0.014000000432133675f,0.004399999976158142f,
                          9.999999747378752e-05f,0.001953125f,0.998046875f,
                          0.751953125f,0.998046875f);
        break;
    case 2:
        // 0x42246B..0x4224A7 -> 0x404620. PAUS / pause plaque.
        b.addBeveledPrism(-0.00800000037997961f,0.0010000000474974513f,
                          -0.003000000026077032f,0.01600000075995922f,
                          0.006000000052154064f,0.0010000000474974513f,
                          0.251953125f,0.498046875f,0.439453125f,0.529296875f);
        break;
    case 3:
        // 0x4224AC..0x4224E8 -> 0x404620. GOVE / "Game Over" plaque,
        // atlas4 UV region x=3.5..252.5, y=.5..47.5.
        b.addBeveledPrism(-0.014999999664723873f,0.0010000000474974513f,
                          -0.003000000026077032f,0.029999999329447746f,
                          0.006000000052154064f,0.0010000000474974513f,
                          0.013671875f,0.986328125f,0.001953125f,0.185546875f);
        break;
    case 4:
        // 0x4224EA..0x42251E -> 0x4041A0. ABOR / abort-confirm plaque.
        b.addBeveledPlane(-0.017999999225139618f,0.f,-0.0020000000949949026f,
                          0.035999998450279236f,0.004000000189989805f,
                          0.00039999998989515007f,0.001953125f,0.998046875f,
                          0.3828125f,0.4990234375f);
        break;
    case 5:
        // 0x422525..0x422554 -> 0x4041A0. GAME/GAM2 flat beveled plaque.
        b.addBeveledPlane(-0.007000000216066837f,0.f,-0.003000000026077032f,
                          0.014000000432133675f,0.006000000052154064f,
                          0.0010000000474974513f,0.001953125f,0.498046875f,
                          0.501953125f,0.685546875f);
        break;
    default:
        break;
    }
    return b.take();
}



} // namespace LegacyModelFactory

namespace LegacyModelFactory {
LegacyMainMenuLogoParts buildMainMenuLogoParts(bool /*highQuality*/) {
    LegacyMainMenuLogoParts out{};
    // r194 DIRECT EXE 0x4220F9..0x4221A9, master 0x02585A60. r166..r193 had
    // copied the 0x422844 menu-slot-4 recipe (r=.023, 18 segments, thin
    // annuli) here; that is why the title showed only a thin ring. LOGO is a
    // 48-segment coin of radius .005 whose two faces carry the whole LOGO
    // texture (uv centre .5/.5, radius .5):
    //   0x402A50({(.005,+.0003125),(.005,-.0003125)},2, 0,0, 48,
    //            .5,.5, .00390625,.00390625, 1)
    //   0x403400(1.6493362, 48, 0,+.0003125,0, .005, .5,.5,.5)
    //   0x4036E0(1.6493362, 48, 0,-.0003125,0, .005, .5,.5,.5)
    LegacyMeshBuilder logo;
    logo.addRevolvedProfile({{0.005f,+0.0003125f},{0.005f,-0.0003125f}},
                            0.f,0.f,48,
                            0.5f,0.5f,0.00390625f,0.00390625f,1.0f);
    constexpr float kLogoTilt=1.6493362188339233f;
    logo.addBeveledCircularCap(0.f,+0.0003125f,0.f,0.005f,48,0.5f,0.5f,0.5f,kLogoTilt,false);
    logo.addBeveledCircularCap(0.f,-0.0003125f,0.f,0.005f,48,0.5f,0.5f,0.5f,kLogoTilt,true);
    out.logo=logo.take();

    // r194 DIRECT EXE 0x4221CF..0x422200, master 0x02585AD8. r166..r193 used
    // the 0x4228F2 menu-slot-5 arguments. Literal 0x404620 tuple:
    //   (-.02, .0004, -.02, .04, .04, .001, 0,1, 0,1)
    LegacyMeshBuilder laxy;
    laxy.addBeveledPrism(-0.02f,0.0004f,-0.02f,0.04f,0.04f,0.001f,
                         0.f,1.f,0.f,1.f);
    out.laxy=laxy.take();
    return out;
}

LegacyMesh buildMainMenuDecorationSlot(int slot,bool highQuality) {
    slot=std::clamp(slot,0,6);
    LegacyMeshBuilder b;
    constexpr float hp=1.5707963267948966f;
    constexpr float uv0=0.126953125f, uv1=0.154296875f;
    constexpr float vv0=0.095703125f, vv1=0.123046875f;
    if(slot==0){
        b.addRevolvedProfile({{.01f,.5f},{.07f,.4f},{.07f,.39f},{.07f,-.39f},{.07f,-.4f},{.01f,-.5f}},
                             hp,-hp,highQuality?18:12,uv0,uv1,vv0,vv1,.1f);
    } else if(slot==1){
        std::vector<LegacyProfilePoint> p; p.reserve(7);
        for(int i=0;i<7;++i){const float a=float(i)*-1.0471975803375244f;p.push_back({std::cos(a)*.0007f+.0035f,std::sin(a)*.0007f});}
        b.addRevolvedProfile(p,0.f,0.f,highQuality?32:20,uv0,uv1,vv0,vv1,10.f);
    } else if(slot==2){
        b.addRevolvedProfile({{.01f,.5f},{.09f,.4f},{.09f,.39f},{.09f,-.39f},{.09f,-.4f},{.01f,-.5f}},
                             hp,-hp,highQuality?18:10,uv0,uv1,vv0,vv1,.08f);
    } else if(slot==3){
        b.addRevolvedProfile({{.01f,.6f},{.07f,.5f},{.07f,.49f},{.07f,-.34f},{.07f,-.35f},{.01f,-.4f}},
                             hp,-hp,highQuality?18:10,uv0,uv1,vv0,vv1,.1f);
    } else if(slot==4){
        // Literal 0x422844..0x4228E8. The old x86 helper shares the bevel-ring
        // boundary with the preceding lathe; the native equivalent preserves
        // the exact profile/radii/heights/UV domains while emitting own quads.
        b.addRevolvedProfile({{.023f,+.002875f},{.023f,-.002875f}},
                             +.039269909262657166f,-.039269909262657166f,18,
                             0.f,.49609375f,.9765625f,.99609375f,1.f);
        // r194 DIRECT EXE 0x42288D..0x4228E3: bevelled caps, not annuli.
        constexpr float kSlot4Tilt=1.7671458721160889f;
        b.addBeveledCircularCap(0.f,+.002875f,0.f,.023f,18,.125f,.8828125f,.12109375f,kSlot4Tilt,false);
        b.addBeveledCircularCap(0.f,-.002875f,0.f,.023f,18,.375f,.875f,.12109375f,kSlot4Tilt,true);
    } else if(slot==5){
        // Literal 0x404620 arguments at 0x4228F2..0x422924.
        b.addBeveledPrism(-.02f,.001f,-.02f,.04f,.04f,.005f,
                          .001953125f,.998046875f,.001953125f,.998046875f);
    } else {
        // Literal 0x4041A0 arguments at 0x422930..0x422962.
        b.addBeveledPlane(-.003f,.0001f,-.0004f,.006f,.0008f,.0001f,
                          .001953125f,.998046875f,.001953125f,.181640625f);
    }
    return b.take();
}
}
