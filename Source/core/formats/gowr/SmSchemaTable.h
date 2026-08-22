#pragma once

// smschema field table, extracted from GoWR.exe.
//
// God of War Ragnarok describes its serialisable data with an in-house
// reflection system its own error strings name "smschema" (Sony Santa Monica
// schema -- see the path in that message: Shared/DataLayer/LibCore/
// core_library_info.cpp). The executable carries the entire table as static
// data: every field of every schema struct, with byte offset, size and type.
//
// -- Record layout in the binary (32 bytes) -------------------------------
//
//   +0x00  u64  namePtr          field name, as a C string
//   +0x08  u64  namePtr          repeated -- this doubling identifies a record
//   +0x10  u16  fieldOffset      byte offset within the owning struct
//   +0x12  u16  size             field size in bytes
//   +0x14  u16  typeCode         see SmType below
//   +0x16  u16  ownerStructId    which struct the field belongs to
//   +0x1A  u16  fieldId          global field index
//
// -- What is NOT here, and why --------------------------------------------
//
// Struct NAMES. The binary holds 3132 strings shaped "namespace.TypeName"
// (core.Vector3, creatureeditor.DrivenBlendNodeData, ...) in a separate
// region, but nothing found so far links ownerStructId to one of them: the id
// is not stored in the struct record, does not index that table positionally,
// and no pointer runs from a struct record into this field table. The link
// most likely lives in the library registrar -- the function that emits the
// "Too many smschema library informations are registered" error -- which is
// where to look next. Until then a struct is identified by its id.
//
// Extraction is reproducible: tools/dump_smschema.java walks the record shape
// above over 0x142000000..0x143000000 and writes the TSV this header is
// generated from. Regenerate when the game patches.
//
// GENERATED -- do not edit by hand.
// Fields: 10474   Structs: 1427   Distinct field names: 7557
// Source: GoWR.exe (PE x86-64, image base 0x140000000)

#include <cstdint>

namespace Onyx::Gowr::SmSchema {

// Type codes as they appear in the table. The meanings are inferred from
// field names and sizes across all records rather than from the game's own
// code, so the ones marked (?) fit every observation but are unconfirmed.
enum class SmType : uint16_t {
    Int32      = 0x0000,   // also enums stored 4 wide
    Float      = 0x0008,   // by far the most common
    Ref        = 0x0010,   // handle/reference to another object (?)
    Bool       = 0x0014,
    IsNullFlag = 0x0016,   // the "<Field>_IsNull" companion of an optional
    StringHash = 0x0018,   // string stored as an 8-byte hash, NOT as text
    Array      = 0x001C,   // (?)
    Struct     = 0x0024,   // (?) embedded struct
    Embedded   = 0x0028,   // (?)
    Vector     = 0x002C,   // colour/vector; size is reported as 0
    Expression = 0x0030,   // (?)
    Enum8      = 0x0104,
    Enum8b     = 0x0105,
    Enum16     = 0x0204,
};

struct Field {
    const char* name;
    uint16_t    offset;
    uint16_t    size;
    uint16_t    type;
    uint16_t    fieldId;
};

struct Struct {
    uint16_t     id;
    uint16_t     fieldCount;
    const Field* fields;
};

inline constexpr Field kFields_0000[] = {
    {"XboxViewButton", 0, 0, 0x0000, 16861},
    {"DrawSimRoot", 0, 0, 0x0001, 0},
    {"DrawContacts", 0, 0, 0x0001, 0},
    {"DrawAABB", 0, 0, 0x0001, 0},
    {"denorm", 0, 0, 0x0001, 0},
    {"Gaussian", 0, 0, 0x0000, 16886},
    {"k900P", 0, 0, 0x0000, 16889},
    {"k30FPS", 0, 0, 0x0000, 16889},
    {"eCMT_Invalid", 0, 0, 0x0000, 16889},
    {"XAxis", 0, 0, 0x0000, 16890},
    {"Linear", 0, 0, 0x0000, 16890},
    {"drawConeOff", 0, 0, 0x0000, 16890},
    {"drawTextOff", 0, 0, 0x0000, 16890},
    {"Min", 0, 0, 0x0000, 16863},
    {"Local", 0, 0, 0x0000, 16861},
    {"Off", 0, 0, 0x0000, 16890},
    {"Shape_Box", 0, 0, 0x0000, 16890},
    {"selected", 0, 0, 0x0000, 16872},
    {"DefaultFlags", 0, 0, 0x0000, 16890},
    {"ColorUnselected", 0, 238, 0x0000, 16890},
    {"CurveType_Hermite", 0, 0, 0x0000, 16890},
    {"LeftStick", 0, 0, 0x0000, 16890},
    {"eDocked", 0, 0, 0x0000, 16891},
    {"eDPForever", 0, 0, 0x0000, 16892},
    {"None", 0, 0, 0x0000, 16892},
    {"HIKSolvingStepRollExtraction", 0, 1, 0x0000, 16892},
    {"HIKSolvingStepLeftArmSnS", 0, 2, 0x0000, 16892},
    {"HIKSolvingStepRightArmSnS", 0, 4, 0x0000, 16892},
    {"HIKSolvingStepLeftLegSnS", 0, 8, 0x0000, 16892},
    {"HIKSolvingStepRightLegSnS", 0, 16, 0x0000, 16892},
    {"HIKSolvingStepModifiers", 0, 32, 0x0000, 16892},
    {"GuassianSqrt", 1, 0, 0x0000, 16865},
    {"k1080P", 1, 0, 0x0000, 16889},
    {"k60FPS", 1, 0, 0x0000, 16889},
    {"eCMT_Block", 1, 0, 0x0000, 16889},
    {"YAxis", 1, 0, 0x0000, 16890},
    {"SmoothStep", 1, 0, 0x0000, 16886},
    {"drawConeOn", 1, 0, 0x0000, 16890},
    {"drawTextOn", 1, 0, 0x0000, 16871},
    {"Max", 1, 0, 0x0000, 16890},
    {"World", 1, 0, 0x0000, 16890},
    {"LPF", 1, 0, 0x0000, 16890},
    {"Shape_Sphere", 1, 0, 0x0000, 16854},
    {"first", 1, 0, 0x0000, 16872},
    {"CurveType_Bezier", 1, 0, 0x0000, 16890},
    {"RightStick", 1, 0, 0x0000, 16890},
    {"eEnteringBoatFromDock", 1, 0, 0x0000, 16891},
    {"eDPActionDuration", 1, 0, 0x0000, 16890},
    {"KPAUnpin", 1, 0, 0x0000, 16892},
    {"HIKSolvingStepBodyPull", 1, 0, 0x0000, 16892},
    {"Active", 1, 0, 0x0000, 16854},
    {"Linear", 2, 0, 0x0000, 16886},
    {"k4K", 2, 0, 0x0000, 16889},
    {"k40FPS", 2, 0, 0x0000, 16889},
    {"eCMT_Evade", 2, 0, 0x0000, 16889},
    {"ZAxis", 2, 0, 0x0000, 16865},
    {"Gaussian", 2, 0, 0x0000, 16890},
    {"Average", 2, 0, 0x0000, 16890},
    {"HPF", 2, 0, 0x0000, 16890},
    {"second", 2, 0, 0x0000, 16890},
    {"eInBoat", 2, 0, 0x0000, 16891},
    {"HIKSolvingStepContact", 2, 0, 0x0000, 16892},
    {"All", 2, 0, 0x0000, 16880},
    {"Quadratic", 3, 0, 0x0000, 16886},
    {"k720P", 3, 0, 0x0000, 16889},
    {"k120FPS", 3, 0, 0x0000, 16889},
    {"eCMT_Parry", 3, 0, 0x0000, 16889},
    {"_kStateCount", 3, 0, 0x0000, 16890},
    {"AnimCurve", 3, 0, 0x0000, 16890},
    {"Multiply", 3, 0, 0x0000, 16890},
    {"APF", 3, 0, 0x0000, 16890},
    {"third", 3, 0, 0x0000, 16890},
    {"CurveType_Bezier_Interactive", 3, 0, 0x0000, 16870},
    {"eDockingOnDock", 3, 0, 0x0000, 16891},
    {"_kDiscoveryTypCount", 3, 0, 0x0000, 16869},
    {"MaxSplitPositions", 4, 0, 0x0000, 16886},
    {"k1440P", 4, 0, 0x0000, 16889},
    {"eCMT_SonGrab_Success", 4, 0, 0x0000, 16889},
    {"GeoAvg", 4, 0, 0x0000, 16871},
    {"BPF", 4, 0, 0x0000, 16884},
    {"last", 4, 0, 0x0000, 16890},
    {"k180UpCW", 4, 0, 0x0000, 16891},
    {"k180Min", 4, 0, 0x0000, 16891},
    {"eEnteringBoatFromBeach", 4, 0, 0x0000, 16891},
    {"HIKSolvingStepContactApprox", 4, 0, 0x0000, 16892},
    {"eCMT_SonGrab_Fail", 5, 0, 0x0000, 16889},
    {"Notch", 5, 0, 0x0000, 16890},
    {"k180UpCCW", 5, 0, 0x0000, 16891},
    {"eDockingOnBeach", 5, 0, 0x0000, 16891},
    {"eCMT_SonFollowUp", 6, 0, 0x0000, 16868},
    {"PEQ", 6, 0, 0x0000, 16890},
    {"k180DownCW", 6, 0, 0x0000, 16891},
    {"eBeached", 6, 0, 0x0000, 16891},
    {"LSH", 7, 0, 0x0000, 16890},
    {"k180DownCCW", 7, 0, 0x0000, 16891},
    {"k180Max", 7, 0, 0x0000, 16891},
    {"HSH", 8, 0, 0x0000, 16890},
    {"k360CW", 8, 0, 0x0000, 16891},
    {"k360Min", 8, 0, 0x0000, 16891},
    {"HIKSolvingStepLeftShoulder", 8, 0, 0x0000, 16892},
    {"Rendering", 9, 0, 0x0000, 16871},
    {"k360CCW", 9, 0, 0x0000, 16868},
    {"k360Max", 9, 0, 0x0000, 16891},
    {"TargetFPS", 15, 0, 0x00F0, 0},
    {"EmitterZoneCount", 16, 0, 0x0000, 16886},
    {"MaxEmitterPriority", 16, 0, 0x0000, 16886},
    {"HIKSolvingStepRightShoulder", 16, 0, 0x0000, 16892},
    {"HIKSolvingStepLeftArm", 32, 0, 0x0000, 16892},
    {"KRNoEvent", 36, 0, 0x0000, 16892},
    {"KREventMismatch", 37, 0, 0x0000, 16892},
    {"HIKSolvingStepRightArm", 64, 0, 0x0000, 16892},
    {"k128", 128, 0, 0x0000, 16890},
    {"HIKSolvingStepLeftLeg", 128, 0, 0x0000, 16892},
    {"ColorHULL", 255, 255, 0x0000, 16890},
    {"k256", 256, 0, 0x0000, 16890},
    {"HIKSolvingStepRightLeg", 256, 0, 0x0000, 16892},
    {"k512", 512, 0, 0x0000, 16890},
    {"HIKSolvingStepLeftHand", 512, 0, 0x0000, 16892},
    {"HIKSolvingStepRightHand", 1024, 0, 0x0000, 16892},
    {"HIKSolvingStepLeftFoot", 2048, 0, 0x0000, 16892},
    {"HIKSolvingStepRightFoot", 4096, 0, 0x0000, 16892},
    {"HIKSolvingStepHead", 8192, 0, 0x0000, 16892},
    {"HIKSolvingStepSpine", 16384, 0, 0x0000, 16892},
    {"HIKSolvingStepAllParts", 32760, 0, 0x0000, 16892},
    {"HIKSolvingStepHipsTranslation", 32768, 0, 0x0000, 16892},
};

inline constexpr Field kFields_0001[] = {
    {"attrVersion", 0, 0, 0x86A0, 0},
};

inline constexpr Field kFields_000B[] = {
    {"Away", 0, 2, 0x0008, 30},
    {"Up", 2, 2, 0x0008, 31},
    {"Right", 4, 2, 0x0008, 32},
    {"Duration", 6, 2, 0x0008, 33},
    {"ImpulseType", 8, 1, 0x0104, 0},
    {"ImpulseRadiusBehavior", 9, 1, 0x0104, 1},
    {"ImpulseFalloff", 10, 1, 0x0104, 2},
    {"ConstantImpulseRadius", 12, 2, 0x0008, 34},
};

inline constexpr Field kFields_000C[] = {
    {"Time", 0, 4, 0x0008, 35},
    {"Distance", 4, 4, 0x0008, 36},
    {"EaseIn", 8, 4, 0x0008, 37},
    {"EaseOut", 12, 4, 0x0008, 38},
    {"LengthIn", 16, 4, 0x0008, 39},
    {"LengthOut", 20, 4, 0x0008, 40},
};

inline constexpr Field kFields_000D[] = {
    {"guid_", 0, 8, 0x0018, 0},
    {"joint_id_", 8, 2, 0x0000, 5},
};

inline constexpr Field kFields_000E[] = {
    {"node_", 0, 8, 0x0020, 15},
    {"in_", 8, 4, 0x0008, 41},
    {"fromMin_", 12, 4, 0x0008, 42},
    {"fromMax_", 16, 4, 0x0008, 43},
    {"toMin_", 20, 4, 0x0008, 44},
    {"toMax_", 24, 4, 0x0008, 45},
    {"remapInput_", 28, 1, 0x0014, 0},
};

inline constexpr Field kFields_000F[] = {
    {"input0_", 0, 0, 0x002C, 14},
    {"input1_", 32, 0, 0x002C, 14},
    {"input2_", 64, 0, 0x002C, 14},
    {"input3_", 96, 0, 0x002C, 14},
    {"input4_", 128, 0, 0x002C, 14},
    {"input5_", 160, 0, 0x002C, 14},
    {"input6_", 192, 0, 0x002C, 14},
    {"input7_", 224, 0, 0x002C, 14},
    {"output0_out_", 256, 4, 0x0008, 86},
    {"num_outputs", 260, 4, 0x0000, 6},
    {"num_inputs", 264, 4, 0x0000, 7},
};

inline constexpr Field kFields_0010[] = {
    {"input8_", 272, 0, 0x002C, 14},
    {"input9_", 304, 0, 0x002C, 14},
    {"input10_", 336, 0, 0x002C, 14},
    {"input11_", 368, 0, 0x002C, 14},
    {"input12_", 400, 0, 0x002C, 14},
    {"input13_", 432, 0, 0x002C, 14},
    {"input14_", 464, 0, 0x002C, 14},
    {"input15_", 496, 0, 0x002C, 14},
    {"input16_", 528, 0, 0x002C, 14},
    {"input17_", 560, 0, 0x002C, 14},
    {"input18_", 592, 0, 0x002C, 14},
    {"input19_", 624, 0, 0x002C, 14},
    {"input20_", 656, 0, 0x002C, 14},
    {"input21_", 688, 0, 0x002C, 14},
    {"input22_", 720, 0, 0x002C, 14},
    {"input23_", 752, 0, 0x002C, 14},
    {"input24_", 784, 0, 0x002C, 14},
    {"input25_", 816, 0, 0x002C, 14},
    {"input26_", 848, 0, 0x002C, 14},
    {"input27_", 880, 0, 0x002C, 14},
    {"input28_", 912, 0, 0x002C, 14},
    {"input29_", 944, 0, 0x002C, 14},
    {"input30_", 976, 0, 0x002C, 14},
    {"input31_", 1008, 0, 0x002C, 14},
    {"input32_", 1040, 0, 0x002C, 14},
    {"input33_", 1072, 0, 0x002C, 14},
    {"input34_", 1104, 0, 0x002C, 14},
    {"input35_", 1136, 0, 0x002C, 14},
    {"input36_", 1168, 0, 0x002C, 14},
    {"input37_", 1200, 0, 0x002C, 14},
    {"input38_", 1232, 0, 0x002C, 14},
    {"input39_", 1264, 0, 0x002C, 14},
    {"input40_", 1296, 0, 0x002C, 14},
    {"input41_", 1328, 0, 0x002C, 14},
    {"input42_", 1360, 0, 0x002C, 14},
    {"input43_", 1392, 0, 0x002C, 14},
    {"input44_", 1424, 0, 0x002C, 14},
    {"input45_", 1456, 0, 0x002C, 14},
    {"input46_", 1488, 0, 0x002C, 14},
    {"input47_", 1520, 0, 0x002C, 14},
    {"input48_", 1552, 0, 0x002C, 14},
    {"input49_", 1584, 0, 0x002C, 14},
    {"input50_", 1616, 0, 0x002C, 14},
    {"input51_", 1648, 0, 0x002C, 14},
    {"input52_", 1680, 0, 0x002C, 14},
    {"input53_", 1712, 0, 0x002C, 14},
    {"input54_", 1744, 0, 0x002C, 14},
    {"input55_", 1776, 0, 0x002C, 14},
    {"input56_", 1808, 0, 0x002C, 14},
    {"input57_", 1840, 0, 0x002C, 14},
    {"input58_", 1872, 0, 0x002C, 14},
    {"input59_", 1904, 0, 0x002C, 14},
    {"input60_", 1936, 0, 0x002C, 14},
    {"input61_", 1968, 0, 0x002C, 14},
    {"input62_", 2000, 0, 0x002C, 14},
    {"input63_", 2032, 0, 0x002C, 14},
    {"input64_", 2064, 0, 0x002C, 14},
    {"input65_", 2096, 0, 0x002C, 14},
    {"input66_", 2128, 0, 0x002C, 14},
    {"input67_", 2160, 0, 0x002C, 14},
    {"input68_", 2192, 0, 0x002C, 14},
    {"input69_", 2224, 0, 0x002C, 14},
    {"input70_", 2256, 0, 0x002C, 14},
    {"input71_", 2288, 0, 0x002C, 14},
    {"input72_", 2320, 0, 0x002C, 14},
    {"input73_", 2352, 0, 0x002C, 14},
    {"input74_", 2384, 0, 0x002C, 14},
    {"input75_", 2416, 0, 0x002C, 14},
    {"input76_", 2448, 0, 0x002C, 14},
    {"input77_", 2480, 0, 0x002C, 14},
    {"input78_", 2512, 0, 0x002C, 14},
    {"input79_", 2544, 0, 0x002C, 14},
    {"input80_", 2576, 0, 0x002C, 14},
    {"input81_", 2608, 0, 0x002C, 14},
    {"input82_", 2640, 0, 0x002C, 14},
    {"input83_", 2672, 0, 0x002C, 14},
    {"input84_", 2704, 0, 0x002C, 14},
    {"input85_", 2736, 0, 0x002C, 14},
    {"input86_", 2768, 0, 0x002C, 14},
    {"input87_", 2800, 0, 0x002C, 14},
    {"input88_", 2832, 0, 0x002C, 14},
    {"input89_", 2864, 0, 0x002C, 14},
    {"input90_", 2896, 0, 0x002C, 14},
    {"input91_", 2928, 0, 0x002C, 14},
    {"input92_", 2960, 0, 0x002C, 14},
    {"input93_", 2992, 0, 0x002C, 14},
    {"input94_", 3024, 0, 0x002C, 14},
    {"input95_", 3056, 0, 0x002C, 14},
    {"input96_", 3088, 0, 0x002C, 14},
    {"input97_", 3120, 0, 0x002C, 14},
    {"input98_", 3152, 0, 0x002C, 14},
    {"input99_", 3184, 0, 0x002C, 14},
    {"input100_", 3216, 0, 0x002C, 14},
    {"input101_", 3248, 0, 0x002C, 14},
    {"input102_", 3280, 0, 0x002C, 14},
    {"input103_", 3312, 0, 0x002C, 14},
    {"input104_", 3344, 0, 0x002C, 14},
    {"input105_", 3376, 0, 0x002C, 14},
    {"input106_", 3408, 0, 0x002C, 14},
    {"input107_", 3440, 0, 0x002C, 14},
    {"input108_", 3472, 0, 0x002C, 14},
    {"input109_", 3504, 0, 0x002C, 14},
    {"input110_", 3536, 0, 0x002C, 14},
    {"input111_", 3568, 0, 0x002C, 14},
    {"input112_", 3600, 0, 0x002C, 14},
    {"input113_", 3632, 0, 0x002C, 14},
    {"input114_", 3664, 0, 0x002C, 14},
    {"input115_", 3696, 0, 0x002C, 14},
    {"input116_", 3728, 0, 0x002C, 14},
    {"input117_", 3760, 0, 0x002C, 14},
    {"input118_", 3792, 0, 0x002C, 14},
    {"input119_", 3824, 0, 0x002C, 14},
    {"input120_", 3856, 0, 0x002C, 14},
    {"input121_", 3888, 0, 0x002C, 14},
    {"input122_", 3920, 0, 0x002C, 14},
    {"input123_", 3952, 0, 0x002C, 14},
    {"input124_", 3984, 0, 0x002C, 14},
    {"input125_", 4016, 0, 0x002C, 14},
    {"input126_", 4048, 0, 0x002C, 14},
    {"input127_", 4080, 0, 0x002C, 14},
    {"output1_out_", 4112, 4, 0x0008, 728},
    {"output2_out_", 4116, 4, 0x0008, 729},
    {"output3_out_", 4120, 4, 0x0008, 730},
    {"output4_out_", 4124, 4, 0x0008, 731},
    {"output5_out_", 4128, 4, 0x0008, 732},
    {"output6_out_", 4132, 4, 0x0008, 733},
    {"output7_out_", 4136, 4, 0x0008, 734},
    {"output8_out_", 4140, 4, 0x0008, 735},
    {"output9_out_", 4144, 4, 0x0008, 736},
    {"output10_out_", 4148, 4, 0x0008, 737},
    {"output11_out_", 4152, 4, 0x0008, 738},
    {"output12_out_", 4156, 4, 0x0008, 739},
    {"output13_out_", 4160, 4, 0x0008, 740},
    {"output14_out_", 4164, 4, 0x0008, 741},
    {"output15_out_", 4168, 4, 0x0008, 742},
    {"output16_out_", 4172, 4, 0x0008, 743},
    {"output17_out_", 4176, 4, 0x0008, 744},
    {"output18_out_", 4180, 4, 0x0008, 745},
    {"output19_out_", 4184, 4, 0x0008, 746},
    {"output20_out_", 4188, 4, 0x0008, 747},
    {"output21_out_", 4192, 4, 0x0008, 748},
    {"output22_out_", 4196, 4, 0x0008, 749},
    {"output23_out_", 4200, 4, 0x0008, 750},
    {"output24_out_", 4204, 4, 0x0008, 751},
    {"output25_out_", 4208, 4, 0x0008, 752},
    {"output26_out_", 4212, 4, 0x0008, 753},
    {"output27_out_", 4216, 4, 0x0008, 754},
    {"output28_out_", 4220, 4, 0x0008, 755},
    {"output29_out_", 4224, 4, 0x0008, 756},
    {"output30_out_", 4228, 4, 0x0008, 757},
    {"output31_out_", 4232, 4, 0x0008, 758},
    {"output32_out_", 4236, 4, 0x0008, 759},
    {"output33_out_", 4240, 4, 0x0008, 760},
    {"output34_out_", 4244, 4, 0x0008, 761},
    {"output35_out_", 4248, 4, 0x0008, 762},
    {"output36_out_", 4252, 4, 0x0008, 763},
    {"output37_out_", 4256, 4, 0x0008, 764},
    {"output38_out_", 4260, 4, 0x0008, 765},
    {"output39_out_", 4264, 4, 0x0008, 766},
    {"output40_out_", 4268, 4, 0x0008, 767},
    {"output41_out_", 4272, 4, 0x0008, 768},
    {"output42_out_", 4276, 4, 0x0008, 769},
    {"output43_out_", 4280, 4, 0x0008, 770},
    {"output44_out_", 4284, 4, 0x0008, 771},
    {"output45_out_", 4288, 4, 0x0008, 772},
    {"output46_out_", 4292, 4, 0x0008, 773},
    {"output47_out_", 4296, 4, 0x0008, 774},
    {"output48_out_", 4300, 4, 0x0008, 775},
    {"output49_out_", 4304, 4, 0x0008, 776},
    {"output50_out_", 4308, 4, 0x0008, 777},
    {"output51_out_", 4312, 4, 0x0008, 778},
    {"output52_out_", 4316, 4, 0x0008, 779},
    {"output53_out_", 4320, 4, 0x0008, 780},
    {"output54_out_", 4324, 4, 0x0008, 781},
    {"output55_out_", 4328, 4, 0x0008, 782},
    {"output56_out_", 4332, 4, 0x0008, 783},
    {"output57_out_", 4336, 4, 0x0008, 784},
    {"output58_out_", 4340, 4, 0x0008, 785},
    {"output59_out_", 4344, 4, 0x0008, 786},
    {"output60_out_", 4348, 4, 0x0008, 787},
    {"output61_out_", 4352, 4, 0x0008, 788},
    {"output62_out_", 4356, 4, 0x0008, 789},
    {"output63_out_", 4360, 4, 0x0008, 790},
    {"output64_out_", 4364, 4, 0x0008, 791},
    {"output65_out_", 4368, 4, 0x0008, 792},
    {"output66_out_", 4372, 4, 0x0008, 793},
    {"output67_out_", 4376, 4, 0x0008, 794},
    {"output68_out_", 4380, 4, 0x0008, 795},
    {"output69_out_", 4384, 4, 0x0008, 796},
    {"output70_out_", 4388, 4, 0x0008, 797},
    {"output71_out_", 4392, 4, 0x0008, 798},
    {"output72_out_", 4396, 4, 0x0008, 799},
    {"output73_out_", 4400, 4, 0x0008, 800},
    {"output74_out_", 4404, 4, 0x0008, 801},
    {"output75_out_", 4408, 4, 0x0008, 802},
    {"output76_out_", 4412, 4, 0x0008, 803},
    {"output77_out_", 4416, 4, 0x0008, 804},
    {"output78_out_", 4420, 4, 0x0008, 805},
    {"output79_out_", 4424, 4, 0x0008, 806},
    {"output80_out_", 4428, 4, 0x0008, 807},
    {"output81_out_", 4432, 4, 0x0008, 808},
    {"output82_out_", 4436, 4, 0x0008, 809},
    {"output83_out_", 4440, 4, 0x0008, 810},
    {"output84_out_", 4444, 4, 0x0008, 811},
    {"output85_out_", 4448, 4, 0x0008, 812},
    {"output86_out_", 4452, 4, 0x0008, 813},
    {"output87_out_", 4456, 4, 0x0008, 814},
    {"output88_out_", 4460, 4, 0x0008, 815},
    {"output89_out_", 4464, 4, 0x0008, 816},
    {"output90_out_", 4468, 4, 0x0008, 817},
    {"output91_out_", 4472, 4, 0x0008, 818},
    {"output92_out_", 4476, 4, 0x0008, 819},
    {"output93_out_", 4480, 4, 0x0008, 820},
    {"output94_out_", 4484, 4, 0x0008, 821},
    {"output95_out_", 4488, 4, 0x0008, 822},
    {"output96_out_", 4492, 4, 0x0008, 823},
    {"output97_out_", 4496, 4, 0x0008, 824},
    {"output98_out_", 4500, 4, 0x0008, 825},
    {"output99_out_", 4504, 4, 0x0008, 826},
    {"output100_out_", 4508, 4, 0x0008, 827},
    {"output101_out_", 4512, 4, 0x0008, 828},
    {"output102_out_", 4516, 4, 0x0008, 829},
    {"output103_out_", 4520, 4, 0x0008, 830},
    {"output104_out_", 4524, 4, 0x0008, 831},
    {"output105_out_", 4528, 4, 0x0008, 832},
    {"output106_out_", 4532, 4, 0x0008, 833},
    {"output107_out_", 4536, 4, 0x0008, 834},
    {"output108_out_", 4540, 4, 0x0008, 835},
    {"output109_out_", 4544, 4, 0x0008, 836},
    {"output110_out_", 4548, 4, 0x0008, 837},
    {"output111_out_", 4552, 4, 0x0008, 838},
    {"output112_out_", 4556, 4, 0x0008, 839},
    {"output113_out_", 4560, 4, 0x0008, 840},
    {"output114_out_", 4564, 4, 0x0008, 841},
    {"output115_out_", 4568, 4, 0x0008, 842},
    {"output116_out_", 4572, 4, 0x0008, 843},
    {"output117_out_", 4576, 4, 0x0008, 844},
    {"output118_out_", 4580, 4, 0x0008, 845},
    {"output119_out_", 4584, 4, 0x0008, 846},
    {"output120_out_", 4588, 4, 0x0008, 847},
    {"output121_out_", 4592, 4, 0x0008, 848},
    {"output122_out_", 4596, 4, 0x0008, 849},
    {"output123_out_", 4600, 4, 0x0008, 850},
    {"output124_out_", 4604, 4, 0x0008, 851},
    {"output125_out_", 4608, 4, 0x0008, 852},
    {"output126_out_", 4612, 4, 0x0008, 853},
    {"output127_out_", 4616, 4, 0x0008, 854},
};

inline constexpr Field kFields_0011[] = {
    {"input8_", 272, 0, 0x002C, 14},
    {"input9_", 304, 0, 0x002C, 14},
    {"input10_", 336, 0, 0x002C, 14},
    {"input11_", 368, 0, 0x002C, 14},
    {"input12_", 400, 0, 0x002C, 14},
    {"input13_", 432, 0, 0x002C, 14},
    {"input14_", 464, 0, 0x002C, 14},
    {"input15_", 496, 0, 0x002C, 14},
    {"input16_", 528, 0, 0x002C, 14},
    {"input17_", 560, 0, 0x002C, 14},
    {"input18_", 592, 0, 0x002C, 14},
    {"input19_", 624, 0, 0x002C, 14},
    {"input20_", 656, 0, 0x002C, 14},
    {"input21_", 688, 0, 0x002C, 14},
    {"input22_", 720, 0, 0x002C, 14},
    {"input23_", 752, 0, 0x002C, 14},
    {"input24_", 784, 0, 0x002C, 14},
    {"input25_", 816, 0, 0x002C, 14},
    {"input26_", 848, 0, 0x002C, 14},
    {"input27_", 880, 0, 0x002C, 14},
    {"input28_", 912, 0, 0x002C, 14},
    {"input29_", 944, 0, 0x002C, 14},
    {"input30_", 976, 0, 0x002C, 14},
    {"input31_", 1008, 0, 0x002C, 14},
    {"material_source_", 1040, 8, 0x0018, 0},
    {"material_guid_", 1048, 8, 0x0018, 0},
    {"output1_out_", 1056, 4, 0x0008, 1016},
    {"output2_out_", 1060, 4, 0x0008, 1017},
    {"output3_out_", 1064, 4, 0x0008, 1018},
    {"output4_out_", 1068, 4, 0x0008, 1019},
    {"output5_out_", 1072, 4, 0x0008, 1020},
    {"output6_out_", 1076, 4, 0x0008, 1021},
    {"output7_out_", 1080, 4, 0x0008, 1022},
    {"output8_out_", 1084, 4, 0x0008, 1023},
    {"output9_out_", 1088, 4, 0x0008, 1024},
    {"output10_out_", 1092, 4, 0x0008, 1025},
    {"output11_out_", 1096, 4, 0x0008, 1026},
    {"output12_out_", 1100, 4, 0x0008, 1027},
    {"output13_out_", 1104, 4, 0x0008, 1028},
    {"output14_out_", 1108, 4, 0x0008, 1029},
    {"output15_out_", 1112, 4, 0x0008, 1030},
    {"output16_out_", 1116, 4, 0x0008, 1031},
    {"output17_out_", 1120, 4, 0x0008, 1032},
    {"output18_out_", 1124, 4, 0x0008, 1033},
    {"output19_out_", 1128, 4, 0x0008, 1034},
    {"output20_out_", 1132, 4, 0x0008, 1035},
    {"output21_out_", 1136, 4, 0x0008, 1036},
    {"output22_out_", 1140, 4, 0x0008, 1037},
    {"output23_out_", 1144, 4, 0x0008, 1038},
    {"output24_out_", 1148, 4, 0x0008, 1039},
    {"output25_out_", 1152, 4, 0x0008, 1040},
    {"output26_out_", 1156, 4, 0x0008, 1041},
    {"output27_out_", 1160, 4, 0x0008, 1042},
    {"output28_out_", 1164, 4, 0x0008, 1043},
    {"output29_out_", 1168, 4, 0x0008, 1044},
    {"output30_out_", 1172, 4, 0x0008, 1045},
    {"output31_out_", 1176, 4, 0x0008, 1046},
    {"layer_offset_", 1180, 4, 0x0000, 12},
};

inline constexpr Field kFields_0012[] = {
    {"node_", 0, 8, 0x0020, 15},
    {"rbfwidth_", 8, 4, 0x0008, 1047},
    {"rbfkernel_", 12, 1, 0x0104, 3},
};

inline constexpr Field kFields_0013[] = {
    {"input8_", 272, 0, 0x002C, 14},
    {"input9_", 304, 0, 0x002C, 14},
    {"input10_", 336, 0, 0x002C, 14},
    {"input11_", 368, 0, 0x002C, 14},
    {"input12_", 400, 0, 0x002C, 14},
    {"input13_", 432, 0, 0x002C, 14},
    {"input14_", 464, 0, 0x002C, 14},
    {"input15_", 496, 0, 0x002C, 14},
    {"input16_", 528, 0, 0x002C, 14},
    {"input17_", 560, 0, 0x002C, 14},
    {"input18_", 592, 0, 0x002C, 14},
    {"input19_", 624, 0, 0x002C, 14},
    {"input20_", 656, 0, 0x002C, 14},
    {"input21_", 688, 0, 0x002C, 14},
    {"input22_", 720, 0, 0x002C, 14},
    {"input23_", 752, 0, 0x002C, 14},
    {"input24_", 784, 0, 0x002C, 14},
    {"input25_", 816, 0, 0x002C, 14},
    {"input26_", 848, 0, 0x002C, 14},
    {"input27_", 880, 0, 0x002C, 14},
    {"input28_", 912, 0, 0x002C, 14},
    {"input29_", 944, 0, 0x002C, 14},
    {"input30_", 976, 0, 0x002C, 14},
    {"input31_", 1008, 0, 0x002C, 14},
    {"output1_out_", 1040, 4, 0x0008, 1209},
    {"output2_out_", 1044, 4, 0x0008, 1210},
    {"output3_out_", 1048, 4, 0x0008, 1211},
    {"output4_out_", 1052, 4, 0x0008, 1212},
    {"output5_out_", 1056, 4, 0x0008, 1213},
    {"output6_out_", 1060, 4, 0x0008, 1214},
    {"output7_out_", 1064, 4, 0x0008, 1215},
    {"output8_out_", 1068, 4, 0x0008, 1216},
    {"output9_out_", 1072, 4, 0x0008, 1217},
    {"output10_out_", 1076, 4, 0x0008, 1218},
    {"output11_out_", 1080, 4, 0x0008, 1219},
    {"output12_out_", 1084, 4, 0x0008, 1220},
    {"output13_out_", 1088, 4, 0x0008, 1221},
    {"output14_out_", 1092, 4, 0x0008, 1222},
    {"output15_out_", 1096, 4, 0x0008, 1223},
    {"output16_out_", 1100, 4, 0x0008, 1224},
    {"output17_out_", 1104, 4, 0x0008, 1225},
    {"output18_out_", 1108, 4, 0x0008, 1226},
    {"output19_out_", 1112, 4, 0x0008, 1227},
    {"output20_out_", 1116, 4, 0x0008, 1228},
    {"output21_out_", 1120, 4, 0x0008, 1229},
    {"output22_out_", 1124, 4, 0x0008, 1230},
    {"output23_out_", 1128, 4, 0x0008, 1231},
    {"output24_out_", 1132, 4, 0x0008, 1232},
    {"output25_out_", 1136, 4, 0x0008, 1233},
    {"output26_out_", 1140, 4, 0x0008, 1234},
    {"output27_out_", 1144, 4, 0x0008, 1235},
    {"output28_out_", 1148, 4, 0x0008, 1236},
    {"output29_out_", 1152, 4, 0x0008, 1237},
    {"output30_out_", 1156, 4, 0x0008, 1238},
    {"output31_out_", 1160, 4, 0x0008, 1239},
    {"output32_out_", 1164, 4, 0x0008, 1240},
    {"output33_out_", 1168, 4, 0x0008, 1241},
    {"output34_out_", 1172, 4, 0x0008, 1242},
    {"output35_out_", 1176, 4, 0x0008, 1243},
    {"output36_out_", 1180, 4, 0x0008, 1244},
    {"output37_out_", 1184, 4, 0x0008, 1245},
    {"output38_out_", 1188, 4, 0x0008, 1246},
    {"output39_out_", 1192, 4, 0x0008, 1247},
    {"output40_out_", 1196, 4, 0x0008, 1248},
    {"output41_out_", 1200, 4, 0x0008, 1249},
    {"output42_out_", 1204, 4, 0x0008, 1250},
    {"output43_out_", 1208, 4, 0x0008, 1251},
    {"output44_out_", 1212, 4, 0x0008, 1252},
    {"output45_out_", 1216, 4, 0x0008, 1253},
    {"output46_out_", 1220, 4, 0x0008, 1254},
    {"output47_out_", 1224, 4, 0x0008, 1255},
    {"output48_out_", 1228, 4, 0x0008, 1256},
    {"output49_out_", 1232, 4, 0x0008, 1257},
    {"output50_out_", 1236, 4, 0x0008, 1258},
    {"output51_out_", 1240, 4, 0x0008, 1259},
    {"output52_out_", 1244, 4, 0x0008, 1260},
    {"output53_out_", 1248, 4, 0x0008, 1261},
    {"output54_out_", 1252, 4, 0x0008, 1262},
    {"output55_out_", 1256, 4, 0x0008, 1263},
    {"output56_out_", 1260, 4, 0x0008, 1264},
    {"output57_out_", 1264, 4, 0x0008, 1265},
    {"output58_out_", 1268, 4, 0x0008, 1266},
    {"output59_out_", 1272, 4, 0x0008, 1267},
    {"output60_out_", 1276, 4, 0x0008, 1268},
    {"output61_out_", 1280, 4, 0x0008, 1269},
    {"output62_out_", 1284, 4, 0x0008, 1270},
    {"output63_out_", 1288, 4, 0x0008, 1271},
    {"output64_out_", 1292, 4, 0x0008, 1272},
    {"output65_out_", 1296, 4, 0x0008, 1273},
    {"output66_out_", 1300, 4, 0x0008, 1274},
    {"output67_out_", 1304, 4, 0x0008, 1275},
    {"output68_out_", 1308, 4, 0x0008, 1276},
    {"output69_out_", 1312, 4, 0x0008, 1277},
    {"output70_out_", 1316, 4, 0x0008, 1278},
    {"output71_out_", 1320, 4, 0x0008, 1279},
    {"output72_out_", 1324, 4, 0x0008, 1280},
    {"output73_out_", 1328, 4, 0x0008, 1281},
    {"output74_out_", 1332, 4, 0x0008, 1282},
    {"output75_out_", 1336, 4, 0x0008, 1283},
    {"output76_out_", 1340, 4, 0x0008, 1284},
    {"output77_out_", 1344, 4, 0x0008, 1285},
    {"output78_out_", 1348, 4, 0x0008, 1286},
    {"output79_out_", 1352, 4, 0x0008, 1287},
    {"output80_out_", 1356, 4, 0x0008, 1288},
    {"output81_out_", 1360, 4, 0x0008, 1289},
    {"output82_out_", 1364, 4, 0x0008, 1290},
    {"output83_out_", 1368, 4, 0x0008, 1291},
    {"output84_out_", 1372, 4, 0x0008, 1292},
    {"output85_out_", 1376, 4, 0x0008, 1293},
    {"output86_out_", 1380, 4, 0x0008, 1294},
    {"output87_out_", 1384, 4, 0x0008, 1295},
    {"output88_out_", 1388, 4, 0x0008, 1296},
    {"output89_out_", 1392, 4, 0x0008, 1297},
    {"output90_out_", 1396, 4, 0x0008, 1298},
    {"output91_out_", 1400, 4, 0x0008, 1299},
    {"output92_out_", 1404, 4, 0x0008, 1300},
    {"output93_out_", 1408, 4, 0x0008, 1301},
    {"output94_out_", 1412, 4, 0x0008, 1302},
    {"output95_out_", 1416, 4, 0x0008, 1303},
    {"output96_out_", 1420, 4, 0x0008, 1304},
    {"output97_out_", 1424, 4, 0x0008, 1305},
    {"output98_out_", 1428, 4, 0x0008, 1306},
    {"output99_out_", 1432, 4, 0x0008, 1307},
    {"output100_out_", 1436, 4, 0x0008, 1308},
    {"output101_out_", 1440, 4, 0x0008, 1309},
    {"output102_out_", 1444, 4, 0x0008, 1310},
    {"output103_out_", 1448, 4, 0x0008, 1311},
    {"output104_out_", 1452, 4, 0x0008, 1312},
    {"output105_out_", 1456, 4, 0x0008, 1313},
    {"output106_out_", 1460, 4, 0x0008, 1314},
    {"output107_out_", 1464, 4, 0x0008, 1315},
    {"output108_out_", 1468, 4, 0x0008, 1316},
    {"output109_out_", 1472, 4, 0x0008, 1317},
    {"output110_out_", 1476, 4, 0x0008, 1318},
    {"output111_out_", 1480, 4, 0x0008, 1319},
    {"output112_out_", 1484, 4, 0x0008, 1320},
    {"output113_out_", 1488, 4, 0x0008, 1321},
    {"output114_out_", 1492, 4, 0x0008, 1322},
    {"output115_out_", 1496, 4, 0x0008, 1323},
    {"output116_out_", 1500, 4, 0x0008, 1324},
    {"output117_out_", 1504, 4, 0x0008, 1325},
    {"output118_out_", 1508, 4, 0x0008, 1326},
    {"output119_out_", 1512, 4, 0x0008, 1327},
    {"output120_out_", 1516, 4, 0x0008, 1328},
    {"output121_out_", 1520, 4, 0x0008, 1329},
    {"output122_out_", 1524, 4, 0x0008, 1330},
    {"output123_out_", 1528, 4, 0x0008, 1331},
    {"output124_out_", 1532, 4, 0x0008, 1332},
    {"output125_out_", 1536, 4, 0x0008, 1333},
    {"output126_out_", 1540, 4, 0x0008, 1334},
    {"output127_out_", 1544, 4, 0x0008, 1335},
    {"num_samples_", 1548, 4, 0x0000, 15},
    {"sample0_node_", 1552, 8, 0x0020, 15},
    {"sample1_node_", 1560, 8, 0x0020, 15},
    {"sample2_node_", 1568, 8, 0x0020, 15},
    {"sample3_node_", 1576, 8, 0x0020, 15},
    {"sample4_node_", 1584, 8, 0x0020, 15},
    {"sample5_node_", 1592, 8, 0x0020, 15},
    {"sample6_node_", 1600, 8, 0x0020, 15},
    {"sample7_node_", 1608, 8, 0x0020, 15},
    {"sample8_node_", 1616, 8, 0x0020, 15},
    {"sample9_node_", 1624, 8, 0x0020, 15},
    {"sample10_node_", 1632, 8, 0x0020, 15},
    {"sample11_node_", 1640, 8, 0x0020, 15},
    {"sample12_node_", 1648, 8, 0x0020, 15},
    {"sample13_node_", 1656, 8, 0x0020, 15},
    {"sample14_node_", 1664, 8, 0x0020, 15},
    {"sample15_node_", 1672, 8, 0x0020, 15},
};

inline constexpr Field kFields_0014[] = {
    {"kernelFunc", 272, 1, 0x0104, 4},
    {"isBase", 273, 1, 0x0014, 209},
    {"TargetValue0", 276, 4, 0x0008, 1377},
    {"TargetValue1", 280, 4, 0x0008, 1378},
    {"TargetValue2", 284, 4, 0x0008, 1379},
    {"TargetValue3", 288, 4, 0x0008, 1380},
    {"TargetValue4", 292, 4, 0x0008, 1381},
    {"TargetValue5", 296, 4, 0x0008, 1382},
    {"TargetValue6", 300, 4, 0x0008, 1383},
    {"TargetValue7", 304, 4, 0x0008, 1384},
    {"TargetValue8", 308, 4, 0x0008, 1385},
    {"TargetValue9", 312, 4, 0x0008, 1386},
    {"TargetValue10", 316, 4, 0x0008, 1387},
    {"TargetValue11", 320, 4, 0x0008, 1388},
    {"TargetValue12", 324, 4, 0x0008, 1389},
    {"TargetValue13", 328, 4, 0x0008, 1390},
    {"TargetValue14", 332, 4, 0x0008, 1391},
    {"TargetValue15", 336, 4, 0x0008, 1392},
    {"TargetValue16", 340, 4, 0x0008, 1393},
    {"TargetValue17", 344, 4, 0x0008, 1394},
    {"TargetValue18", 348, 4, 0x0008, 1395},
    {"TargetValue19", 352, 4, 0x0008, 1396},
    {"TargetValue20", 356, 4, 0x0008, 1397},
    {"TargetValue21", 360, 4, 0x0008, 1398},
    {"TargetValue22", 364, 4, 0x0008, 1399},
    {"TargetValue23", 368, 4, 0x0008, 1400},
    {"TargetValue24", 372, 4, 0x0008, 1401},
    {"TargetValue25", 376, 4, 0x0008, 1402},
    {"TargetValue26", 380, 4, 0x0008, 1403},
    {"TargetValue27", 384, 4, 0x0008, 1404},
    {"TargetValue28", 388, 4, 0x0008, 1405},
    {"TargetValue29", 392, 4, 0x0008, 1406},
    {"TargetValue30", 396, 4, 0x0008, 1407},
    {"TargetValue31", 400, 4, 0x0008, 1408},
    {"TargetValue32", 404, 4, 0x0008, 1409},
    {"TargetValue33", 408, 4, 0x0008, 1410},
    {"TargetValue34", 412, 4, 0x0008, 1411},
    {"TargetValue35", 416, 4, 0x0008, 1412},
    {"TargetValue36", 420, 4, 0x0008, 1413},
    {"TargetValue37", 424, 4, 0x0008, 1414},
    {"TargetValue38", 428, 4, 0x0008, 1415},
    {"TargetValue39", 432, 4, 0x0008, 1416},
    {"TargetValue40", 436, 4, 0x0008, 1417},
    {"TargetValue41", 440, 4, 0x0008, 1418},
    {"TargetValue42", 444, 4, 0x0008, 1419},
    {"TargetValue43", 448, 4, 0x0008, 1420},
    {"TargetValue44", 452, 4, 0x0008, 1421},
    {"TargetValue45", 456, 4, 0x0008, 1422},
    {"TargetValue46", 460, 4, 0x0008, 1423},
    {"TargetValue47", 464, 4, 0x0008, 1424},
    {"TargetValue48", 468, 4, 0x0008, 1425},
    {"TargetValue49", 472, 4, 0x0008, 1426},
    {"TargetValue50", 476, 4, 0x0008, 1427},
    {"TargetValue51", 480, 4, 0x0008, 1428},
    {"TargetValue52", 484, 4, 0x0008, 1429},
    {"TargetValue53", 488, 4, 0x0008, 1430},
    {"TargetValue54", 492, 4, 0x0008, 1431},
    {"TargetValue55", 496, 4, 0x0008, 1432},
    {"TargetValue56", 500, 4, 0x0008, 1433},
    {"TargetValue57", 504, 4, 0x0008, 1434},
    {"TargetValue58", 508, 4, 0x0008, 1435},
    {"TargetValue59", 512, 4, 0x0008, 1436},
    {"TargetValue60", 516, 4, 0x0008, 1437},
    {"TargetValue61", 520, 4, 0x0008, 1438},
    {"TargetValue62", 524, 4, 0x0008, 1439},
    {"TargetValue63", 528, 4, 0x0008, 1440},
    {"TargetValue64", 532, 4, 0x0008, 1441},
    {"TargetValue65", 536, 4, 0x0008, 1442},
    {"TargetValue66", 540, 4, 0x0008, 1443},
    {"TargetValue67", 544, 4, 0x0008, 1444},
    {"TargetValue68", 548, 4, 0x0008, 1445},
    {"TargetValue69", 552, 4, 0x0008, 1446},
    {"TargetValue70", 556, 4, 0x0008, 1447},
    {"TargetValue71", 560, 4, 0x0008, 1448},
    {"TargetValue72", 564, 4, 0x0008, 1449},
    {"TargetValue73", 568, 4, 0x0008, 1450},
    {"TargetValue74", 572, 4, 0x0008, 1451},
    {"TargetValue75", 576, 4, 0x0008, 1452},
    {"TargetValue76", 580, 4, 0x0008, 1453},
    {"TargetValue77", 584, 4, 0x0008, 1454},
    {"TargetValue78", 588, 4, 0x0008, 1455},
    {"TargetValue79", 592, 4, 0x0008, 1456},
    {"TargetValue80", 596, 4, 0x0008, 1457},
    {"TargetValue81", 600, 4, 0x0008, 1458},
    {"TargetValue82", 604, 4, 0x0008, 1459},
    {"TargetValue83", 608, 4, 0x0008, 1460},
    {"TargetValue84", 612, 4, 0x0008, 1461},
    {"TargetValue85", 616, 4, 0x0008, 1462},
    {"TargetValue86", 620, 4, 0x0008, 1463},
    {"TargetValue87", 624, 4, 0x0008, 1464},
    {"TargetValue88", 628, 4, 0x0008, 1465},
    {"TargetValue89", 632, 4, 0x0008, 1466},
    {"TargetValue90", 636, 4, 0x0008, 1467},
    {"TargetValue91", 640, 4, 0x0008, 1468},
    {"TargetValue92", 644, 4, 0x0008, 1469},
    {"TargetValue93", 648, 4, 0x0008, 1470},
    {"TargetValue94", 652, 4, 0x0008, 1471},
    {"TargetValue95", 656, 4, 0x0008, 1472},
    {"TargetValue96", 660, 4, 0x0008, 1473},
    {"TargetValue97", 664, 4, 0x0008, 1474},
    {"TargetValue98", 668, 4, 0x0008, 1475},
    {"TargetValue99", 672, 4, 0x0008, 1476},
    {"TargetValue100", 676, 4, 0x0008, 1477},
    {"TargetValue101", 680, 4, 0x0008, 1478},
    {"TargetValue102", 684, 4, 0x0008, 1479},
    {"TargetValue103", 688, 4, 0x0008, 1480},
    {"TargetValue104", 692, 4, 0x0008, 1481},
    {"TargetValue105", 696, 4, 0x0008, 1482},
    {"TargetValue106", 700, 4, 0x0008, 1483},
    {"TargetValue107", 704, 4, 0x0008, 1484},
    {"TargetValue108", 708, 4, 0x0008, 1485},
    {"TargetValue109", 712, 4, 0x0008, 1486},
    {"TargetValue110", 716, 4, 0x0008, 1487},
    {"TargetValue111", 720, 4, 0x0008, 1488},
    {"TargetValue112", 724, 4, 0x0008, 1489},
    {"TargetValue113", 728, 4, 0x0008, 1490},
    {"TargetValue114", 732, 4, 0x0008, 1491},
    {"TargetValue115", 736, 4, 0x0008, 1492},
    {"TargetValue116", 740, 4, 0x0008, 1493},
    {"TargetValue117", 744, 4, 0x0008, 1494},
    {"TargetValue118", 748, 4, 0x0008, 1495},
    {"TargetValue119", 752, 4, 0x0008, 1496},
    {"TargetValue120", 756, 4, 0x0008, 1497},
    {"TargetValue121", 760, 4, 0x0008, 1498},
    {"TargetValue122", 764, 4, 0x0008, 1499},
    {"TargetValue123", 768, 4, 0x0008, 1500},
    {"TargetValue124", 772, 4, 0x0008, 1501},
    {"TargetValue125", 776, 4, 0x0008, 1502},
    {"TargetValue126", 780, 4, 0x0008, 1503},
    {"TargetValue127", 784, 4, 0x0008, 1504},
    {"rbfWidth", 788, 4, 0x0008, 1505},
    {"driver0_", 792, 4, 0x0008, 1506},
    {"driver1_", 796, 4, 0x0008, 1507},
    {"driver2_", 800, 4, 0x0008, 1508},
    {"driver3_", 804, 4, 0x0008, 1509},
    {"driver4_", 808, 4, 0x0008, 1510},
    {"driver5_", 812, 4, 0x0008, 1511},
    {"driver6_", 816, 4, 0x0008, 1512},
    {"driver7_", 820, 4, 0x0008, 1513},
    {"driver8_", 824, 4, 0x0008, 1514},
    {"driver9_", 828, 4, 0x0008, 1515},
    {"driver10_", 832, 4, 0x0008, 1516},
    {"driver11_", 836, 4, 0x0008, 1517},
    {"driver12_", 840, 4, 0x0008, 1518},
    {"driver13_", 844, 4, 0x0008, 1519},
    {"driver14_", 848, 4, 0x0008, 1520},
    {"driver15_", 852, 4, 0x0008, 1521},
    {"driver16_", 856, 4, 0x0008, 1522},
    {"driver17_", 860, 4, 0x0008, 1523},
    {"driver18_", 864, 4, 0x0008, 1524},
    {"driver19_", 868, 4, 0x0008, 1525},
    {"driver20_", 872, 4, 0x0008, 1526},
    {"driver21_", 876, 4, 0x0008, 1527},
    {"driver22_", 880, 4, 0x0008, 1528},
    {"driver23_", 884, 4, 0x0008, 1529},
    {"driver24_", 888, 4, 0x0008, 1530},
    {"driver25_", 892, 4, 0x0008, 1531},
    {"driver26_", 896, 4, 0x0008, 1532},
    {"driver27_", 900, 4, 0x0008, 1533},
    {"driver28_", 904, 4, 0x0008, 1534},
    {"driver29_", 908, 4, 0x0008, 1535},
    {"driver30_", 912, 4, 0x0008, 1536},
    {"driver31_", 916, 4, 0x0008, 1537},
};

inline constexpr Field kFields_0015[] = {
    {"joint_input_", 272, 0, 0x002C, 13},
    {"readAxis", 288, 1, 0x0104, 5},
    {"interpMode", 292, 4, 0x0104, 6},
    {"allowRotate", 296, 4, 0x0008, 1579},
    {"minAngle", 300, 4, 0x0008, 1580},
    {"maxAngle", 304, 4, 0x0008, 1581},
    {"allowTwist", 308, 4, 0x0008, 1582},
    {"minTwist", 312, 4, 0x0008, 1583},
    {"maxTwist", 316, 4, 0x0008, 1584},
    {"allowTranslate", 320, 4, 0x0008, 1585},
    {"minTranslate", 324, 4, 0x0008, 1586},
    {"maxTranslate", 328, 4, 0x0008, 1587},
    {"drawCone", 332, 1, 0x0104, 7},
    {"drawText", 333, 1, 0x0104, 8},
    {"drawDetail", 334, 1, 0x0000, 21},
    {"drawReverse", 335, 1, 0x0014, 218},
    {"drawHighlight", 336, 4, 0x0008, 1588},
    {"drawSize", 340, 4, 0x0008, 1589},
    {"lineWidth", 344, 4, 0x0008, 1590},
};

inline constexpr Field kFields_0016[] = {
    {"envelope", 272, 4, 0x0008, 1632},
    {"triggerMode", 276, 1, 0x0104, 9},
};

inline constexpr Field kFields_0017[] = {
    {"pos0_", 272, 0, 0x002C, 13},
    {"pos1_", 288, 0, 0x002C, 13},
    {"output1_out_", 304, 4, 0x0008, 1674},
    {"output2_out_", 308, 4, 0x0008, 1675},
    {"output3_out_", 312, 4, 0x0008, 1676},
};

inline constexpr Field kFields_0018[] = {
    {"joint_input_", 272, 0, 0x002C, 13},
    {"output1_out_", 288, 4, 0x0008, 1718},
    {"output2_out_", 292, 4, 0x0008, 1719},
    {"output3_out_", 296, 4, 0x0008, 1720},
    {"output4_out_", 300, 4, 0x0008, 1721},
    {"output5_out_", 304, 4, 0x0008, 1722},
    {"output6_out_", 308, 4, 0x0008, 1723},
    {"output7_out_", 312, 4, 0x0008, 1724},
    {"output8_out_", 316, 4, 0x0008, 1725},
    {"Translate", 320, 1, 0x0014, 243},
    {"Scale", 321, 1, 0x0014, 244},
    {"Rotate", 322, 1, 0x0014, 245},
    {"transformSpace", 323, 1, 0x0104, 10},
};

inline constexpr Field kFields_0019[] = {
    {"input8_", 272, 0, 0x002C, 14},
    {"input9_", 304, 0, 0x002C, 14},
    {"input10_", 336, 0, 0x002C, 14},
    {"input11_", 368, 0, 0x002C, 14},
    {"input12_", 400, 0, 0x002C, 14},
    {"input13_", 432, 0, 0x002C, 14},
    {"input14_", 464, 0, 0x002C, 14},
    {"input15_", 496, 0, 0x002C, 14},
    {"input16_", 528, 0, 0x002C, 14},
    {"input17_", 560, 0, 0x002C, 14},
    {"input18_", 592, 0, 0x002C, 14},
    {"input19_", 624, 0, 0x002C, 14},
    {"input20_", 656, 0, 0x002C, 14},
    {"input21_", 688, 0, 0x002C, 14},
    {"input22_", 720, 0, 0x002C, 14},
    {"input23_", 752, 0, 0x002C, 14},
    {"input24_", 784, 0, 0x002C, 14},
    {"input25_", 816, 0, 0x002C, 14},
    {"input26_", 848, 0, 0x002C, 14},
    {"Translate", 880, 4, 0x0000, 33},
    {"Rotate", 884, 4, 0x0000, 34},
    {"Scale", 888, 4, 0x0000, 35},
    {"NumTransforms", 892, 4, 0x0000, 36},
    {"joint_output_", 896, 0, 0x002C, 13},
};

inline constexpr Field kFields_001A[] = {
    {"PortalName", 0, 8, 0x0018, 0},
    {"PortalGUID", 8, 8, 0x0018, 0},
    {"PortalIsGameObject", 16, 1, 0x0014, 273},
};

inline constexpr Field kFields_001B[] = {
    {"EmitterName", 0, 8, 0x0018, 0},
    {"EmitterGUID", 8, 8, 0x0018, 0},
    {"EmitterIsGameObject", 16, 1, 0x0014, 274},
};

inline constexpr Field kFields_001C[] = {
    {"ObjectName", 0, 8, 0x0018, 0},
    {"GameObjectGUID", 8, 8, 0x0018, 0},
    {"IsGameObject", 16, 1, 0x0014, 275},
};

inline constexpr Field kFields_001D[] = {
    {"LinkName", 0, 8, 0x0018, 0},
    {"LinkGUID", 8, 8, 0x0018, 0},
    {"LinkIsGameObject", 16, 1, 0x0014, 276},
};

inline constexpr Field kFields_001E[] = {
    {"Zones", 0, 12, 0x0024, 0},
    {"ZoneCount", 12, 4, 0x0000, 38},
    {"Portals", 16, 12, 0x0024, 1},
    {"EmitterCount", 28, 4, 0x0000, 39},
    {"Emitters", 32, 12, 0x0024, 2},
    {"PortalCount", 44, 4, 0x0000, 40},
    {"Links", 48, 12, 0x0024, 3},
    {"LinkCount", 60, 4, 0x0000, 41},
    {"ZoneIndexIntoEmitterArray", 64, 12, 0x0024, 4},
    {"LinkIndexStartForZone", 80, 12, 0x0024, 5},
};

inline constexpr Field kFields_001F[] = {
    {"Placeholder", 0, 1, 0x0014, 277},
};

inline constexpr Field kFields_0020[] = {
    {"RTPCName", 0, 8, 0x0018, 0},
    {"RTPCFunction", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0021[] = {
    {"ContainingZoneOverrides", 0, 384, 0x0024, 6},
    {"Sound", 384, 16, 0x0024, 7},
    {"RTPCData", 400, 12, 0x0024, 8},
    {"Radius", 412, 4, 0x0008, 1862},
    {"HealthRTPCNameOverride", 416, 8, 0x0018, 0},
    {"VelocityRTPCNameOverride", 424, 8, 0x0018, 0},
    {"AngXVelRTPCNameOverride", 432, 8, 0x0018, 0},
    {"AngYVelRTPCNameOverride", 440, 8, 0x0018, 0},
    {"ResponseEvent", 448, 8, 0x0018, 0},
    {"Curve", 456, 8, 0x0018, 0},
    {"PlaneSize", 464, 0, 0x002C, 4},
    {"ObstructionRate", 472, 4, 0x0008, 1865},
    {"SeperatedChannelsEmittersRadius", 476, 4, 0x0008, 1866},
    {"SeperatedChannelCollapseDistanceStart", 480, 4, 0x0008, 1867},
    {"SeperatedChannelCollapseDistanceEnd", 484, 4, 0x0008, 1868},
    {"RTPCCount", 488, 4, 0x0000, 44},
    {"RaycastStartPositionOffset", 492, 4, 0x0008, 1869},
    {"CullRadius", 496, 4, 0x0008, 1870},
    {"CullRadiusMinimum", 500, 4, 0x0008, 1871},
    {"Play", 504, 1, 0x0014, 278},
    {"StopOnEmitterDestroy", 505, 1, 0x0014, 279},
    {"AnimationDirectionFilter", 506, 1, 0x0104, 11},
    {"ExportName", 507, 1, 0x0014, 280},
    {"ZoneActivated", 508, 1, 0x0014, 281},
    {"AnimationDriven", 509, 1, 0x0014, 282},
    {"Obstructable", 510, 1, 0x0014, 283},
    {"ObstructionCasts", 511, 1, 0x0000, 45},
    {"ObstructionPriority", 512, 1, 0x0000, 46},
    {"SplitPositions", 513, 1, 0x0000, 47},
    {"UseStandardRTPCs", 514, 1, 0x0014, 284},
    {"UseIsIndoorsRTPC", 515, 1, 0x0014, 285},
    {"UseWindIntensityRTPC", 516, 1, 0x0014, 286},
    {"UseWindGustRTPC", 517, 1, 0x0014, 287},
    {"UseHealthRTPC", 518, 1, 0x0014, 288},
    {"UseVelocityRTPC", 519, 1, 0x0014, 289},
    {"UseAngXVelRTPC", 520, 1, 0x0014, 290},
    {"UseAngYVelRTPC", 521, 1, 0x0014, 291},
    {"UseDistanceToCameraRTPC", 522, 1, 0x0014, 292},
    {"SendObstructionOcclusionAsRTPC", 523, 1, 0x0014, 293},
    {"IsLightOneEmitter", 524, 1, 0x0014, 294},
    {"IsBreakableEmitter", 525, 1, 0x0014, 295},
    {"NeverCull", 526, 1, 0x0014, 296},
    {"NeverOcclude", 527, 1, 0x0014, 297},
    {"NonRepositioning", 528, 1, 0x0014, 298},
    {"ObstructionParent", 529, 1, 0x0014, 299},
    {"ObstructionChild", 530, 1, 0x0014, 300},
    {"EnableEarlyReflections", 531, 1, 0x0014, 301},
    {"EarlyReflectionsParent", 532, 1, 0x0014, 302},
    {"EarlyReflectionsChild", 533, 1, 0x0014, 303},
    {"LockEmitterFacingWhilePlaying", 534, 1, 0x0014, 304},
    {"IsSeparatedChannelsEmitter", 535, 1, 0x0014, 305},
    {"UsePlane", 536, 1, 0x0014, 306},
    {"CustomCullRadius", 537, 1, 0x0014, 307},
    {"EnableOcclusion", 538, 1, 0x0014, 308},
    {"EnableDiffraction", 539, 1, 0x0014, 309},
    {"EnableVirtualPositions", 540, 1, 0x0014, 310},
    {"EnableReverb", 541, 1, 0x0014, 311},
    {"OffsetInCameraSpace", 542, 1, 0x0000, 48},
};

inline constexpr Field kFields_0022[] = {
    {"Mode", 0, 1, 0x0104, 12},
    {"Gain", 4, 4, 0x0008, 1872},
    {"Freq", 8, 4, 0x0008, 1873},
};

inline constexpr Field kFields_0023[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Environment", 8, 80, 0x0024, 9},
    {"Ambience", 88, 8, 0x0010, 0},
    {"Priority", 96, 1, 0x0004, 13},
    {"CrossFadeDistance", 100, 4, 0x0008, 1875},
    {"CrossFadeInside", 104, 1, 0x0014, 312},
    {"RegionsOutsideOcclusion", 108, 4, 0x0008, 1876},
    {"ContainingRoom", 112, 0, 0x002C, 28},
};

inline constexpr Field kFields_0024[] = {
    {"PortalTweakName", 0, 8, 0x0018, 0},
    {"StartEnabled", 8, 1, 0x0014, 314},
};

inline constexpr Field kFields_0025[] = {
    {"Portals", 0, 12, 0x0024, 10},
    {"PrimaryPortal", 12, 4, 0x0000, 49},
};

inline constexpr Field kFields_0026[] = {
    {"ThreatScalar", 0, 4, 0x0008, 1877},
};

inline constexpr Field kFields_0027[] = {
    {"Sound", 0, 8, 0x0018, 0},
    {"SoundEmitter", 8, 8, 0x0018, 0},
    {"TriggerRange", 16, 4, 0x0008, 1878},
    {"TriggeredBy", 20, 1, 0x0104, 14},
    {"ResetTriggerWhenOutOfRange", 21, 1, 0x0014, 315},
    {"StartEnabled", 22, 1, 0x0014, 316},
};

inline constexpr Field kFields_0028[] = {
    {"EventSound", 0, 8, 0x0018, 0},
    {"EventFrame", 8, 4, 0x0000, 50},
};

inline constexpr Field kFields_0029[] = {
    {"Events", 0, 12, 0x0024, 11},
    {"DistanceCull", 12, 4, 0x0008, 1879},
    {"SoundEmitter", 16, 8, 0x0018, 0},
    {"EventCount", 24, 4, 0x0000, 51},
};

inline constexpr Field kFields_002A[] = {
    {"Profile", 12, 1, 0x0204, 15},
};

inline constexpr Field kFields_002B[] = {
    {"Start", 0, 2, 0x0008, 1883},
    {"End", 2, 2, 0x0008, 1884},
};

inline constexpr Field kFields_002C[] = {
    {"Min", 0, 2, 0x0008, 1885},
    {"Center", 2, 2, 0x0008, 1886},
    {"Max", 4, 2, 0x0008, 1887},
};

inline constexpr Field kFields_002D[] = {
    {"Left", 0, 2, 0x0008, 1888},
    {"Right", 2, 2, 0x0008, 1889},
    {"Down", 4, 2, 0x0008, 1890},
    {"Up", 6, 2, 0x0008, 1891},
    {"Backward", 8, 2, 0x0008, 1892},
    {"Forward", 10, 2, 0x0008, 1893},
};

inline constexpr Field kFields_002E[] = {
    {"ID", 0, 4, 0x0001, 52},
    {"Type", 4, 4, 0x0105, 16},
    {"TweenDriver", 8, 1, 0x0104, 17},
    {"Hold", 9, 1, 0x0014, 317},
    {"Priority", 12, 4, 0x0000, 53},
    {"Weight", 16, 4, 0x0008, 1894},
    {"TweenIn", 20, 0, 0x002C, 12},
    {"Duration", 44, 4, 0x0008, 1901},
    {"TweenOut", 48, 0, 0x002C, 12},
    {"Script", 72, 8, 0x0018, 0},
};

inline constexpr Field kFields_002F[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"AutoFirstSplitDistance", 88, 4, 0x0008, 1922},
    {"AutoLastSplitDistance", 92, 4, 0x0008, 1923},
    {"Split1Distance", 96, 4, 0x0008, 1924},
    {"Split2Distance", 100, 4, 0x0008, 1925},
    {"Split3Distance", 104, 4, 0x0008, 1926},
    {"Split4Distance", 108, 4, 0x0008, 1927},
    {"Split5Distance", 112, 4, 0x0008, 1928},
    {"Split6Distance", 116, 4, 0x0008, 1929},
    {"Split7Distance", 120, 4, 0x0008, 1930},
    {"LightCameraOffsetLightDirection", 124, 4, 0x0008, 1931},
    {"LightCameraRange", 128, 4, 0x0008, 1932},
    {"LodDistanceScale", 132, 4, 0x0008, 1933},
    {"CascadeHeadResolutionScale", 136, 4, 0x0008, 1934},
    {"CascadeTailResolutionScale", 140, 4, 0x0008, 1935},
    {"NumCascades", 144, 1, 0x0000, 56},
    {"NumCascades_IsNull", 145, 1, 0x0016, 319},
    {"NumDynamicCascades", 146, 1, 0x0000, 57},
    {"NumDynamicCascades_IsNull", 147, 1, 0x0016, 320},
    {"AutoFirstSplitDistance_IsNull", 148, 1, 0x0016, 321},
    {"AutoLastSplitDistance_IsNull", 149, 1, 0x0016, 322},
    {"Split1Distance_IsNull", 150, 1, 0x0016, 323},
    {"Split2Distance_IsNull", 151, 1, 0x0016, 324},
    {"Split3Distance_IsNull", 152, 1, 0x0016, 325},
    {"Split4Distance_IsNull", 153, 1, 0x0016, 326},
    {"Split5Distance_IsNull", 154, 1, 0x0016, 327},
    {"Split6Distance_IsNull", 155, 1, 0x0016, 328},
    {"Split7Distance_IsNull", 156, 1, 0x0016, 329},
    {"LightCameraOffsetLightDirection_IsNull", 157, 1, 0x0016, 330},
    {"LightCameraRange_IsNull", 158, 1, 0x0016, 331},
    {"LodDistanceScale_IsNull", 159, 1, 0x0016, 332},
    {"CascadeHeadResolutionScale_IsNull", 160, 1, 0x0016, 333},
    {"CascadeTailResolutionScale_IsNull", 161, 1, 0x0016, 334},
};

inline constexpr Field kFields_0030[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"SSAOStrength", 88, 4, 0x0008, 1950},
    {"SSAORadius", 92, 4, 0x0008, 1951},
    {"SSAOPowerExponent", 96, 4, 0x0008, 1952},
    {"CapsuleStrength", 100, 4, 0x0008, 1953},
    {"SSAOStrength_IsNull", 104, 1, 0x0016, 336},
    {"SSAORadius_IsNull", 105, 1, 0x0016, 337},
    {"SSAOPowerExponent_IsNull", 106, 1, 0x0016, 338},
    {"CapsuleStrength_IsNull", 107, 1, 0x0016, 339},
};

inline constexpr Field kFields_0031[] = {
    {"ExposureBiasRamp", 80, 12, 0x0024, 12},
    {"ExposureBias", 92, 4, 0x0008, 1968},
    {"TemplateSymbol", 96, 8, 0x001A, 0},
    {"ExposureMin", 104, 4, 0x0008, 1969},
    {"ExposureMax", 108, 4, 0x0008, 1970},
    {"LocalAdaptationShadows", 112, 4, 0x0008, 1971},
    {"LocalAdaptationHighlights", 116, 4, 0x0008, 1972},
    {"WhiteBalanceMin", 120, 4, 0x0008, 1973},
    {"SecondsBrightToDark", 124, 4, 0x0008, 1974},
    {"SecondsDarkToBright", 128, 4, 0x0008, 1975},
    {"AutoExposureCharacterWeight", 132, 4, 0x0008, 1976},
    {"AutoExposureDepthWeight", 136, 4, 0x0008, 1977},
    {"AutoExposureDepthWeightExponent", 140, 4, 0x0008, 1978},
    {"ExposureBias_IsNull", 144, 1, 0x0016, 341},
    {"ExposureBiasRamp_IsNull", 145, 1, 0x0016, 342},
    {"ExposureMin_IsNull", 146, 1, 0x0016, 343},
    {"ExposureMax_IsNull", 147, 1, 0x0016, 344},
    {"LocalAdaptationShadows_IsNull", 148, 1, 0x0016, 345},
    {"LocalAdaptationHighlights_IsNull", 149, 1, 0x0016, 346},
    {"WhiteBalanceMin_IsNull", 150, 1, 0x0016, 347},
    {"TonemappingCurve", 151, 1, 0x0104, 24},
    {"TonemappingCurve_IsNull", 152, 1, 0x0016, 348},
    {"SecondsBrightToDark_IsNull", 153, 1, 0x0016, 349},
    {"SecondsDarkToBright_IsNull", 154, 1, 0x0016, 350},
    {"AutoExposureCharacterWeight_IsNull", 155, 1, 0x0016, 351},
    {"AutoExposureDepthWeight_IsNull", 156, 1, 0x0016, 352},
    {"AutoExposureDepthWeightExponent_IsNull", 157, 1, 0x0016, 353},
};

inline constexpr Field kFields_0032[] = {
    {"Shadows", 80, 0, 0x002C, 1427},
    {"Midtones", 92, 0, 0x002C, 1427},
    {"Highlights", 104, 0, 0x002C, 1427},
    {"Vibrance", 116, 4, 0x0008, 2002},
    {"TemplateSymbol", 120, 8, 0x001A, 0},
    {"LutFile", 128, 8, 0x0018, 0},
    {"LevelsInput", 136, 0, 0x002C, 1426},
    {"LevelsOutput", 144, 0, 0x002C, 1426},
    {"Saturation", 152, 4, 0x0008, 2007},
    {"LevelsGamma", 156, 4, 0x0008, 2008},
    {"LutFile_IsNull", 160, 1, 0x0016, 355},
    {"Vibrance_IsNull", 161, 1, 0x0016, 356},
    {"Saturation_IsNull", 162, 1, 0x0016, 357},
    {"LevelsGamma_IsNull", 163, 1, 0x0016, 358},
    {"LevelsInput_IsNull", 164, 1, 0x0016, 359},
    {"LevelsOutput_IsNull", 165, 1, 0x0016, 360},
    {"Shadows_IsNull", 166, 1, 0x0016, 361},
    {"Midtones_IsNull", 167, 1, 0x0016, 362},
    {"Highlights_IsNull", 168, 1, 0x0016, 363},
    {"PreserveLuminosity", 169, 1, 0x0014, 364},
    {"PreserveLuminosity_IsNull", 170, 1, 0x0016, 365},
};

inline constexpr Field kFields_0033[] = {
    {"VignetteColor", 80, 0, 0x002C, 1},
    {"TemplateSymbol", 96, 8, 0x001A, 0},
    {"Brightness", 104, 4, 0x0008, 2027},
    {"Falloff", 108, 4, 0x0008, 2028},
    {"Scale", 112, 4, 0x0008, 2029},
    {"Brightness_IsNull", 116, 1, 0x0016, 367},
    {"Falloff_IsNull", 117, 1, 0x0016, 368},
    {"Scale_IsNull", 118, 1, 0x0016, 369},
    {"VignetteColor_IsNull", 119, 1, 0x0016, 370},
};

inline constexpr Field kFields_0034[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"VelocityMultiplier", 88, 4, 0x0008, 2044},
    {"VelocityMultiplier_IsNull", 92, 1, 0x0016, 372},
};

inline constexpr Field kFields_0035[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"Amount", 88, 4, 0x0008, 2059},
    {"Amount_IsNull", 92, 1, 0x0016, 374},
};

inline constexpr Field kFields_0036[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"Width", 88, 4, 0x0008, 2074},
    {"Opacity", 92, 4, 0x0008, 2075},
    {"ThresholdRelativeEV", 96, 4, 0x0008, 2076},
    {"AdditionalWidthScale", 100, 4, 0x0008, 2077},
    {"Width_IsNull", 104, 1, 0x0016, 376},
    {"Opacity_IsNull", 105, 1, 0x0016, 377},
    {"ThresholdRelativeEV_IsNull", 106, 1, 0x0016, 378},
    {"AdditionalWidthScale_IsNull", 107, 1, 0x0016, 379},
};

inline constexpr Field kFields_0037[] = {
    {"LightColor", 80, 0, 0x002C, 1},
    {"HazeColor", 96, 0, 0x002C, 1},
    {"TemplateSymbol", 112, 8, 0x001A, 0},
    {"LightIntensity", 120, 4, 0x0008, 2100},
    {"LightElevation", 124, 4, 0x0008, 2101},
    {"LightAngle", 128, 4, 0x0008, 2102},
    {"LightDirectionalityPower", 132, 4, 0x0008, 2103},
    {"HazeIntensity", 136, 4, 0x0008, 2104},
    {"VerticalFalloff", 140, 4, 0x0008, 2105},
    {"VerticalRelativePosition", 144, 4, 0x0008, 2106},
    {"VerticalPosition", 148, 4, 0x0008, 2107},
    {"DensityMultiplier", 152, 4, 0x0008, 2108},
    {"StartDistance", 156, 4, 0x0008, 2109},
    {"OverrideSkyDepth", 160, 4, 0x0008, 2110},
    {"RelativeToSunLight", 164, 1, 0x0014, 381},
    {"RelativeToSunLight_IsNull", 165, 1, 0x0016, 382},
    {"LightColor_IsNull", 166, 1, 0x0016, 383},
    {"LightIntensity_IsNull", 167, 1, 0x0016, 384},
    {"LightElevation_IsNull", 168, 1, 0x0016, 385},
    {"LightAngle_IsNull", 169, 1, 0x0016, 386},
    {"LightDirectionalityPower_IsNull", 170, 1, 0x0016, 387},
    {"HazeColor_IsNull", 171, 1, 0x0016, 388},
    {"HazeIntensity_IsNull", 172, 1, 0x0016, 389},
    {"EnableCameraRelativeHeightFog", 173, 1, 0x0014, 390},
    {"EnableCameraRelativeHeightFog_IsNull", 174, 1, 0x0016, 391},
    {"VerticalFalloff_IsNull", 175, 1, 0x0016, 392},
    {"VerticalRelativePosition_IsNull", 176, 1, 0x0016, 393},
    {"VerticalPosition_IsNull", 177, 1, 0x0016, 394},
    {"DensityMultiplier_IsNull", 178, 1, 0x0016, 395},
    {"StartDistance_IsNull", 179, 1, 0x0016, 396},
    {"OverrideSkyDepth_IsNull", 180, 1, 0x0016, 397},
};

inline constexpr Field kFields_0038[] = {
    {"SunHazeAlbedo", 80, 0, 0x002C, 1},
    {"FogAlbedo", 96, 0, 0x002C, 1},
    {"FogEmissiveTint", 112, 0, 0x002C, 1},
    {"RaymarchFogAlbedo", 128, 0, 0x002C, 1},
    {"RaymarchFogEmissiveTint", 144, 0, 0x002C, 1},
    {"TemplateSymbol", 160, 8, 0x001A, 0},
    {"DistanceLUTTextureFilename", 168, 8, 0x0018, 0},
    {"FogEffectiveDistance", 176, 4, 0x0008, 2145},
    {"Density", 180, 4, 0x0008, 2146},
    {"DensityOpacity", 184, 4, 0x0008, 2147},
    {"DensityMapStrength", 188, 4, 0x0008, 2148},
    {"SunHazeFalloff", 192, 4, 0x0008, 2149},
    {"VerticalFalloff", 196, 4, 0x0008, 2150},
    {"OpacityVerticalFalloff", 200, 4, 0x0008, 2151},
    {"VerticalPosition", 204, 4, 0x0008, 2152},
    {"VerticalRelativePosition", 208, 4, 0x0008, 2153},
    {"FogEmissiveIntensity", 212, 4, 0x0008, 2154},
    {"ExposureAdaptation", 216, 4, 0x0008, 2155},
    {"FogPhaseFunctionG", 220, 4, 0x0008, 2156},
    {"DirectionalLightMultiplier", 224, 4, 0x0008, 2157},
    {"FogStartDistance", 228, 4, 0x0008, 2158},
    {"FogShadowFade", 232, 4, 0x0008, 2159},
    {"MirrorFogPlaneOffset", 236, 4, 0x0008, 2160},
    {"MirrorFogDensityMultiplier", 240, 4, 0x0008, 2161},
    {"ResolutionScale", 244, 4, 0x0008, 2162},
    {"OcclusionTestingMinDistance", 248, 4, 0x0008, 2163},
    {"ConvergenceFactor", 252, 4, 0x0008, 2164},
    {"RaymarchStartDistance", 256, 4, 0x0008, 2165},
    {"RaymarchTransitionDistance", 260, 4, 0x0008, 2166},
    {"RaymarchMaxDistance", 264, 4, 0x0008, 2167},
    {"RaymarchMaxDepthClampDistance", 268, 4, 0x0008, 2168},
    {"RaymarchDensity", 272, 4, 0x0008, 2169},
    {"RaymarchDensityOpacity", 276, 4, 0x0008, 2170},
    {"RaymarchDensityMapStrength", 280, 4, 0x0008, 2171},
    {"RaymarchVerticalFalloff", 284, 4, 0x0008, 2172},
    {"RaymarchOpacityVerticalFalloff", 288, 4, 0x0008, 2173},
    {"RaymarchVerticalPosition", 292, 4, 0x0008, 2174},
    {"RaymarchVerticalRelativePosition", 296, 4, 0x0008, 2175},
    {"RaymarchFogEmissiveIntensity", 300, 4, 0x0008, 2176},
    {"RaymarchExposureAdaptation", 304, 4, 0x0008, 2177},
    {"LUTTextureSwizzle", 308, 4, 0x0000, 76},
    {"DistanceLUTMinDistance", 312, 4, 0x0008, 2178},
    {"DistanceLUTMaxDistance", 316, 4, 0x0008, 2179},
    {"FogEffectiveDistance_IsNull", 320, 1, 0x0016, 399},
    {"Density_IsNull", 321, 1, 0x0016, 400},
    {"DensityOpacity_IsNull", 322, 1, 0x0016, 401},
    {"DensityMapStrength_IsNull", 323, 1, 0x0016, 402},
    {"SunHazeAlbedo_IsNull", 324, 1, 0x0016, 403},
    {"SunHazeFalloff_IsNull", 325, 1, 0x0016, 404},
    {"EnableCameraRelativeHeightFog", 326, 1, 0x0014, 405},
    {"EnableCameraRelativeHeightFog_IsNull", 327, 1, 0x0016, 406},
    {"VerticalFalloff_IsNull", 328, 1, 0x0016, 407},
    {"OpacityVerticalFalloff_IsNull", 329, 1, 0x0016, 408},
    {"VerticalPosition_IsNull", 330, 1, 0x0016, 409},
    {"VerticalRelativePosition_IsNull", 331, 1, 0x0016, 410},
    {"FogAlbedo_IsNull", 332, 1, 0x0016, 411},
    {"FogEmissiveTint_IsNull", 333, 1, 0x0016, 412},
    {"FogEmissiveIntensity_IsNull", 334, 1, 0x0016, 413},
    {"ExposureAdaptation_IsNull", 335, 1, 0x0016, 414},
    {"FogPhaseFunctionG_IsNull", 336, 1, 0x0016, 415},
    {"DirectionalLightMultiplier_IsNull", 337, 1, 0x0016, 416},
    {"FogStartDistance_IsNull", 338, 1, 0x0016, 417},
    {"FogShadowFade_IsNull", 339, 1, 0x0016, 418},
    {"MirrorFogPlaneOffset_IsNull", 340, 1, 0x0016, 419},
    {"MirrorFogDensityMultiplier_IsNull", 341, 1, 0x0016, 420},
    {"ResolutionScale_IsNull", 342, 1, 0x0016, 421},
    {"VolumetricFogTileQuality", 343, 1, 0x0104, 39},
    {"VolumetricFogTileQuality_IsNull", 344, 1, 0x0016, 422},
    {"OcclusionTestingMinDistance_IsNull", 345, 1, 0x0016, 423},
    {"ConvergenceFactor_IsNull", 346, 1, 0x0016, 424},
    {"FallbackToGlobalDensityMap", 347, 1, 0x0014, 425},
    {"FallbackToGlobalDensityMap_IsNull", 348, 1, 0x0016, 426},
    {"EnableRaymarching", 349, 1, 0x0014, 427},
    {"EnableRaymarching_IsNull", 350, 1, 0x0016, 428},
    {"RaymarchMaxStepCount", 351, 1, 0x0000, 77},
    {"RaymarchMaxStepCount_IsNull", 352, 1, 0x0016, 429},
    {"RaymarchStartDistance_IsNull", 353, 1, 0x0016, 430},
    {"RaymarchTransitionDistance_IsNull", 354, 1, 0x0016, 431},
    {"RaymarchMaxDistance_IsNull", 355, 1, 0x0016, 432},
    {"RaymarchMaxDepthClampDistance_IsNull", 356, 1, 0x0016, 433},
    {"UseVolumetricFogSettingsForRaymarch", 357, 1, 0x0014, 434},
    {"UseVolumetricFogSettingsForRaymarch_IsNull", 358, 1, 0x0016, 435},
    {"RaymarchDensity_IsNull", 359, 1, 0x0016, 436},
    {"RaymarchDensityOpacity_IsNull", 360, 1, 0x0016, 437},
    {"RaymarchDensityMapStrength_IsNull", 361, 1, 0x0016, 438},
    {"RaymarchVerticalFalloff_IsNull", 362, 1, 0x0016, 439},
    {"RaymarchOpacityVerticalFalloff_IsNull", 363, 1, 0x0016, 440},
    {"RaymarchVerticalPosition_IsNull", 364, 1, 0x0016, 441},
    {"RaymarchVerticalRelativePosition_IsNull", 365, 1, 0x0016, 442},
    {"RaymarchFogAlbedo_IsNull", 366, 1, 0x0016, 443},
    {"RaymarchFogEmissiveTint_IsNull", 367, 1, 0x0016, 444},
    {"RaymarchFogEmissiveIntensity_IsNull", 368, 1, 0x0016, 445},
    {"RaymarchExposureAdaptation_IsNull", 369, 1, 0x0016, 446},
    {"RaymarchEnableGIVolumes", 370, 1, 0x0014, 447},
    {"RaymarchEnableGIVolumes_IsNull", 371, 1, 0x0016, 448},
    {"DistanceLUTTextureFilename_IsNull", 372, 1, 0x0016, 449},
    {"LUTTextureSwizzle_IsNull", 373, 1, 0x0016, 450},
    {"DistanceLUTMinDistance_IsNull", 374, 1, 0x0016, 451},
    {"DistanceLUTMaxDistance_IsNull", 375, 1, 0x0016, 452},
};

inline constexpr Field kFields_0039[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"EnableSSR", 88, 4, 0x0008, 2194},
    {"EnableSSR_IsNull", 92, 1, 0x0016, 454},
    {"ObjectThickness", 96, 4, 0x0008, 2195},
    {"ObjectThickness_IsNull", 100, 1, 0x0016, 455},
};

inline constexpr Field kFields_003A[] = {
    {"Color", 0, 0, 0x002C, 1},
    {"IntensityScale", 16, 4, 0x0008, 2200},
    {"PositionOffset", 20, 4, 0x0008, 2201},
    {"SizeX", 24, 4, 0x0008, 2202},
    {"SizeY", 28, 4, 0x0008, 2203},
    {"RotationOffset", 32, 2, 0x0008, 2204},
    {"OffsetFromAxis", 34, 2, 0x0008, 2205},
    {"IntensityInterpolationDistance", 36, 2, 0x0008, 2206},
    {"IntensityModulateInner", 38, 2, 0x0008, 2207},
    {"IntensityModulateMiddle", 40, 2, 0x0008, 2208},
    {"IntensityModulateOuter", 42, 2, 0x0008, 2209},
    {"SizeXInterpolationDistance", 44, 2, 0x0008, 2210},
    {"SizeXModulateInner", 46, 2, 0x0008, 2211},
    {"SizeXModulateMiddle", 48, 2, 0x0008, 2212},
    {"SizeXModulateOuter", 50, 2, 0x0008, 2213},
    {"SizeYInterpolationDistance", 52, 2, 0x0008, 2214},
    {"SizeYModulateInner", 54, 2, 0x0008, 2215},
    {"SizeYModulateMiddle", 56, 2, 0x0008, 2216},
    {"SizeYModulateOuter", 58, 2, 0x0008, 2217},
    {"RotationFactor", 60, 1, 0x0000, 80},
    {"ScaleWithOcclusion", 61, 1, 0x0014, 456},
    {"Enabled", 62, 1, 0x0014, 457},
    {"LockAxisX", 63, 1, 0x0014, 458},
    {"LockAxisY", 64, 1, 0x0014, 459},
    {"IgnoreGlobalColor", 65, 1, 0x0014, 460},
    {"TexturePath", 72, 8, 0x0018, 0},
};

inline constexpr Field kFields_003B[] = {
    {"Sprites", 0, 2560, 0x0024, 13},
    {"SpriteCount", 2560, 1, 0x0000, 81},
    {"GlobalIntensityScale", 2564, 4, 0x0008, 2218},
    {"DistanceCutOff", 2568, 4, 0x0008, 2219},
    {"ViewDirIntensityCurve", 2572, 4, 0x0008, 2220},
    {"ExposureAdaptation", 2576, 4, 0x0008, 2221},
    {"OccluderCameraOffset", 2580, 4, 0x0008, 2222},
    {"SunLensFlareOccluderDistance", 2584, 4, 0x0008, 2223},
    {"OccluderOffset", 2588, 4, 0x0008, 2224},
    {"ColorModulate", 2592, 0, 0x002C, 1},
    {"OccluderSize", 2608, 1, 0x0104, 42},
    {"IgnoreDistance", 2609, 1, 0x0014, 461},
    {"IsSunLensFlare", 2610, 1, 0x0014, 462},
    {"UseLensFlareTrimming", 2611, 1, 0x0014, 463},
    {"FollowAttachedLightIntensity", 2612, 1, 0x0014, 464},
    {"FollowAttachedLightColor", 2613, 1, 0x0014, 465},
    {"IsVanaheimSkyLensFlare", 2614, 1, 0x0014, 466},
};

inline constexpr Field kFields_003C[] = {
    {"Color", 0, 0, 0x002C, 1},
    {"ProjectorTextureFilename", 16, 8, 0x0018, 0},
    {"PerLightShadowProxyUUID", 24, 8, 0x0018, 0},
    {"IntensityInEVs", 32, 4, 0x0008, 2233},
    {"AngularSize", 36, 4, 0x0008, 2234},
    {"Size", 40, 4, 0x0008, 2235},
    {"CullingDistance", 44, 4, 0x0008, 2236},
    {"CullingFadeDistance", 48, 4, 0x0008, 2237},
    {"NearCullingDistance", 52, 4, 0x0008, 2238},
    {"NearCullingFadeDistance", 56, 4, 0x0008, 2239},
    {"ExposureAdaptation", 60, 4, 0x0008, 2240},
    {"LightRangeOverride", 64, 4, 0x0008, 2241},
    {"ConeAngle", 68, 4, 0x0008, 2242},
    {"PenumbraAngle", 72, 4, 0x0008, 2243},
    {"Dropoff", 76, 4, 0x0008, 2244},
    {"SourceStartingPoint", 80, 4, 0x0008, 2245},
    {"ShadowCullingDistance", 84, 4, 0x0008, 2246},
    {"ShadowBias", 88, 4, 0x0008, 2247},
    {"FilterSize", 92, 4, 0x0008, 2248},
    {"ShadowCullingNearPlaneOffset", 96, 4, 0x0008, 2249},
    {"ShadowCullingFarPlaneOffset", 100, 4, 0x0008, 2250},
    {"PointShadowFadeTarget", 104, 4, 0x0008, 2251},
    {"PointShadowDepthTarget", 108, 4, 0x0008, 2252},
    {"CacheShadowDistance", 112, 4, 0x0008, 2253},
    {"ProjectorScale", 116, 4, 0x0008, 2254},
    {"ProjectorSwizzle", 120, 4, 0x0000, 82},
    {"ProjectorUVScale", 124, 0, 0x002C, 3},
    {"ProjectorUVOffset", 128, 0, 0x002C, 3},
    {"PointFogFadeAmount", 132, 4, 0x0008, 2259},
    {"SpotFogFadeAmount", 136, 4, 0x0008, 2260},
    {"ColorTemperature", 140, 2, 0x0004, 43},
    {"ShadowResolution", 142, 2, 0x0000, 83},
    {"Type", 144, 1, 0x0104, 44},
    {"BakeType", 145, 1, 0x0104, 45},
    {"ShadowBakeType", 146, 1, 0x0104, 46},
    {"Category", 147, 1, 0x0104, 47},
    {"Layer", 148, 1, 0x0104, 48},
    {"DontAffectFog", 149, 1, 0x0014, 467},
    {"AffectOnlyFog", 150, 1, 0x0014, 468},
    {"DontAffectParticles", 151, 1, 0x0014, 469},
    {"AffectOnlyParticles", 152, 1, 0x0014, 470},
    {"IsMirrorLight", 153, 1, 0x0014, 471},
    {"ApplyEmissiveScale", 154, 1, 0x0014, 472},
    {"DontAffectOpaqueRefraction", 155, 1, 0x0014, 473},
    {"AffectOnlyOpaqueRefraction", 156, 1, 0x0014, 474},
    {"CastDynamicShadow", 157, 1, 0x0014, 475},
    {"CastCapsuleShadow", 158, 1, 0x0014, 476},
    {"CubemapPointShadow", 159, 1, 0x0014, 477},
    {"DualPointShadow", 160, 1, 0x0014, 478},
    {"NumCascades", 161, 1, 0x0000, 84},
    {"NumDynamicCascades", 162, 1, 0x0000, 85},
    {"CacheShadows", 163, 1, 0x0014, 479},
    {"ProjectorTextureFormat", 164, 1, 0x0104, 49},
    {"StaticShadow", 165, 1, 0x0014, 480},
    {"CaptureOnStart", 166, 1, 0x0014, 481},
    {"DynamicCharacters", 167, 1, 0x0014, 482},
    {"NoHeroShadows", 168, 1, 0x0014, 483},
    {"SSDOShadowMode", 169, 1, 0x0104, 50},
    {"HighQualityOnly", 170, 1, 0x0014, 484},
};

inline constexpr Field kFields_003D[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"Default", 88, 4, 0x0008, 2275},
    {"NaturalSunSky", 92, 4, 0x0008, 2276},
    {"InteriorOrArtificial", 96, 4, 0x0008, 2277},
    {"GameplayCombat", 100, 4, 0x0008, 2278},
    {"CinematicKey", 104, 4, 0x0008, 2279},
    {"CinematicRimShaping", 108, 4, 0x0008, 2280},
    {"SpecialFXEnvironmentFX", 112, 4, 0x0008, 2281},
    {"SpecialFXMagic", 116, 4, 0x0008, 2282},
    {"SpecialFXCharacters", 120, 4, 0x0008, 2283},
    {"SpecialFXBosses", 124, 4, 0x0008, 2284},
    {"Bifrost", 128, 4, 0x0008, 2285},
    {"Bifrost2", 132, 4, 0x0008, 2286},
    {"Bifrost3", 136, 4, 0x0008, 2287},
    {"PlayerFillLight", 140, 4, 0x0008, 2288},
    {"VisualLanguage", 144, 4, 0x0008, 2289},
    {"CharacterReadability", 148, 4, 0x0008, 2290},
    {"DirLight", 152, 4, 0x0008, 2291},
    {"Navigation", 156, 4, 0x0008, 2292},
    {"Custom1", 160, 4, 0x0008, 2293},
    {"Custom2", 164, 4, 0x0008, 2294},
    {"Custom3", 168, 4, 0x0008, 2295},
    {"Custom4", 172, 4, 0x0008, 2296},
    {"Custom5", 176, 4, 0x0008, 2297},
    {"CineCustom1", 180, 4, 0x0008, 2298},
    {"CineCustom2", 184, 4, 0x0008, 2299},
    {"CineCustom3", 188, 4, 0x0008, 2300},
    {"CineCustom4", 192, 4, 0x0008, 2301},
    {"CustomFillLight1", 196, 4, 0x0008, 2302},
    {"CustomFillLight2", 200, 4, 0x0008, 2303},
    {"Camera", 204, 4, 0x0008, 2304},
    {"Vehicle", 208, 4, 0x0008, 2305},
    {"CompanionBifrost", 212, 4, 0x0008, 2306},
    {"CompanionFillLight", 216, 4, 0x0008, 2307},
    {"PlayerAtreusBifrost", 220, 4, 0x0008, 2308},
    {"PlayerAtreusBifrost2", 224, 4, 0x0008, 2309},
    {"PlayerAtreusBifrost3", 228, 4, 0x0008, 2310},
    {"PlayerAtreusFillLight", 232, 4, 0x0008, 2311},
    {"CompanionAtreusBifrost", 236, 4, 0x0008, 2312},
    {"CompanionAtreusFillLight", 240, 4, 0x0008, 2313},
    {"Vendor", 244, 4, 0x0008, 2314},
    {"FreyaBifrost", 248, 4, 0x0008, 2315},
    {"Default_IsNull", 252, 1, 0x0016, 486},
    {"NaturalSunSky_IsNull", 253, 1, 0x0016, 487},
    {"InteriorOrArtificial_IsNull", 254, 1, 0x0016, 488},
    {"GameplayCombat_IsNull", 255, 1, 0x0016, 489},
    {"CinematicKey_IsNull", 256, 1, 0x0016, 490},
    {"CinematicRimShaping_IsNull", 257, 1, 0x0016, 491},
    {"SpecialFXEnvironmentFX_IsNull", 258, 1, 0x0016, 492},
    {"SpecialFXMagic_IsNull", 259, 1, 0x0016, 493},
    {"SpecialFXCharacters_IsNull", 260, 1, 0x0016, 494},
    {"SpecialFXBosses_IsNull", 261, 1, 0x0016, 495},
    {"Bifrost_IsNull", 262, 1, 0x0016, 496},
    {"Bifrost2_IsNull", 263, 1, 0x0016, 497},
    {"Bifrost3_IsNull", 264, 1, 0x0016, 498},
    {"PlayerFillLight_IsNull", 265, 1, 0x0016, 499},
    {"VisualLanguage_IsNull", 266, 1, 0x0016, 500},
    {"CharacterReadability_IsNull", 267, 1, 0x0016, 501},
    {"DirLight_IsNull", 268, 1, 0x0016, 502},
    {"Navigation_IsNull", 269, 1, 0x0016, 503},
    {"Custom1_IsNull", 270, 1, 0x0016, 504},
    {"Custom2_IsNull", 271, 1, 0x0016, 505},
    {"Custom3_IsNull", 272, 1, 0x0016, 506},
    {"Custom4_IsNull", 273, 1, 0x0016, 507},
    {"Custom5_IsNull", 274, 1, 0x0016, 508},
    {"CineCustom1_IsNull", 275, 1, 0x0016, 509},
    {"CineCustom2_IsNull", 276, 1, 0x0016, 510},
    {"CineCustom3_IsNull", 277, 1, 0x0016, 511},
    {"CineCustom4_IsNull", 278, 1, 0x0016, 512},
    {"CustomFillLight1_IsNull", 279, 1, 0x0016, 513},
    {"CustomFillLight2_IsNull", 280, 1, 0x0016, 514},
    {"Camera_IsNull", 281, 1, 0x0016, 515},
    {"Vehicle_IsNull", 282, 1, 0x0016, 516},
    {"CompanionBifrost_IsNull", 283, 1, 0x0016, 517},
    {"CompanionFillLight_IsNull", 284, 1, 0x0016, 518},
    {"PlayerAtreusBifrost_IsNull", 285, 1, 0x0016, 519},
    {"PlayerAtreusBifrost2_IsNull", 286, 1, 0x0016, 520},
    {"PlayerAtreusBifrost3_IsNull", 287, 1, 0x0016, 521},
    {"PlayerAtreusFillLight_IsNull", 288, 1, 0x0016, 522},
    {"CompanionAtreusBifrost_IsNull", 289, 1, 0x0016, 523},
    {"CompanionAtreusFillLight_IsNull", 290, 1, 0x0016, 524},
    {"Vendor_IsNull", 291, 1, 0x0016, 525},
    {"FreyaBifrost_IsNull", 292, 1, 0x0016, 526},
};

inline constexpr Field kFields_003E[] = {
    {"Tint", 80, 0, 0x002C, 1},
    {"EnvironmentMapCapturePosition", 96, 0, 0x002C, 7},
    {"InteriorEnvironmentMapCapturePosition", 108, 0, 0x002C, 7},
    {"TemplateSymbol", 120, 8, 0x001A, 0},
    {"EnvironmentMap", 128, 8, 0x0018, 0},
    {"InteriorEnvironmentMap", 136, 8, 0x0018, 0},
    {"EnvironmentMapCaptureResolution", 144, 4, 0x0000, 90},
    {"IntensityInEVs", 148, 4, 0x0008, 2340},
    {"Rotation", 152, 4, 0x0008, 2341},
    {"InteriorEnvironmentMapCaptureResolution", 156, 4, 0x0000, 91},
    {"InteriorIntensityInEVs", 160, 4, 0x0008, 2342},
    {"NoGiOcclusionAmount", 164, 4, 0x0008, 2343},
    {"NoGiOcclusionIsHemisphere", 168, 4, 0x0008, 2344},
    {"UseSky", 172, 1, 0x0014, 528},
    {"UseSky_IsNull", 173, 1, 0x0016, 529},
    {"EnvironmentMap_IsNull", 174, 1, 0x0016, 530},
    {"EnvironmentMapCapturePosition_IsNull", 175, 1, 0x0016, 531},
    {"EnvironmentMapCaptureResolution_IsNull", 176, 1, 0x0016, 532},
    {"Tint_IsNull", 177, 1, 0x0016, 533},
    {"IntensityInEVs_IsNull", 178, 1, 0x0016, 534},
    {"Rotation_IsNull", 179, 1, 0x0016, 535},
    {"InteriorEnvironmentMap_IsNull", 180, 1, 0x0016, 536},
    {"InteriorEnvironmentMapCapturePosition_IsNull", 181, 1, 0x0016, 537},
    {"InteriorEnvironmentMapCaptureResolution_IsNull", 182, 1, 0x0016, 538},
    {"InteriorIntensityInEVs_IsNull", 183, 1, 0x0016, 539},
    {"NoGiOcclusionAmount_IsNull", 184, 1, 0x0016, 540},
    {"NoGiOcclusionIsHemisphere_IsNull", 185, 1, 0x0016, 541},
};

inline constexpr Field kFields_003F[] = {
    {"SunColor", 80, 0, 0x002C, 1},
    {"TemplateSymbol", 96, 8, 0x001A, 0},
    {"ProjectorTextureFilename", 104, 8, 0x0018, 0},
    {"SkyEVIntensity", 112, 4, 0x0008, 2363},
    {"SunAzimuth", 116, 4, 0x0008, 2364},
    {"SunElevation", 120, 4, 0x0008, 2365},
    {"SunEVIntensity", 124, 4, 0x0008, 2366},
    {"SunAngularDiameter", 128, 4, 0x0008, 2367},
    {"SunShadowFilterSize", 132, 4, 0x0008, 2368},
    {"ProjectorSwizzle", 136, 4, 0x0000, 94},
    {"ProjectorScale", 140, 4, 0x0008, 2369},
    {"ProjectorUVScale", 144, 0, 0x002C, 3},
    {"ProjectorUVOffset", 148, 0, 0x002C, 3},
    {"CinematicGIScale", 152, 4, 0x0008, 2374},
    {"SunColorTemperature", 156, 2, 0x0004, 57},
    {"Layer", 158, 1, 0x0104, 58},
    {"Layer_IsNull", 159, 1, 0x0016, 543},
    {"Category", 160, 1, 0x0104, 59},
    {"Category_IsNull", 161, 1, 0x0016, 544},
    {"SkyEVIntensity_IsNull", 162, 1, 0x0016, 545},
    {"SunAzimuth_IsNull", 163, 1, 0x0016, 546},
    {"SunElevation_IsNull", 164, 1, 0x0016, 547},
    {"SunColor_IsNull", 165, 1, 0x0016, 548},
    {"SunColorTemperature_IsNull", 166, 1, 0x0016, 549},
    {"SunEVIntensity_IsNull", 167, 1, 0x0016, 550},
    {"SunAngularDiameter_IsNull", 168, 1, 0x0016, 551},
    {"SunShadowFilterSize_IsNull", 169, 1, 0x0016, 552},
    {"ProjectorTextureFilename_IsNull", 170, 1, 0x0016, 553},
    {"ProjectorTextureFormat", 171, 1, 0x0104, 60},
    {"ProjectorTextureFormat_IsNull", 172, 1, 0x0016, 554},
    {"ProjectorSwizzle_IsNull", 173, 1, 0x0016, 555},
    {"ProjectorScale_IsNull", 174, 1, 0x0016, 556},
    {"ProjectorUVScale_IsNull", 175, 1, 0x0016, 557},
    {"ProjectorUVOffset_IsNull", 176, 1, 0x0016, 558},
    {"CinematicGIScale_IsNull", 177, 1, 0x0016, 559},
};

inline constexpr Field kFields_0040[] = {
    {"CapturePosition", 0, 0, 0x002C, 7},
    {"InfluencePosition", 12, 0, 0x002C, 7},
    {"InfluenceScale", 24, 0, 0x002C, 7},
    {"InfluenceRotation", 36, 0, 0x002C, 7},
    {"GeometryProxyPosition", 48, 0, 0x002C, 7},
    {"GeometryProxyScale", 60, 0, 0x002C, 7},
    {"GeometryProxyRotation", 72, 0, 0x002C, 7},
    {"InfluenceInnerRange", 84, 4, 0x0008, 2396},
    {"Filename", 88, 8, 0x0018, 0},
    {"Resolution", 96, 2, 0x0104, 61},
    {"Layer", 98, 1, 0x0104, 62},
    {"Shape", 99, 1, 0x0104, 63},
};

inline constexpr Field kFields_0041[] = {
    {"Layer", 0, 1, 0x0104, 64},
    {"priority", 4, 4, 0x0008, 2397},
    {"sampleOffset", 8, 4, 0x0008, 2398},
    {"compressedDirectionalMultiplier", 12, 4, 0x0008, 2399},
    {"Scale", 16, 4, 0x0008, 2400},
    {"Tint", 32, 0, 0x002C, 1},
    {"TintTarget", 48, 0, 0x002C, 1},
};

inline constexpr Field kFields_0042[] = {
    {"MinResolution", 0, 1, 0x0000, 95},
    {"MaxResolution", 1, 1, 0x0000, 96},
    {"Priority", 4, 4, 0x0008, 2409},
};

inline constexpr Field kFields_0043[] = {
    {"Layer", 0, 1, 0x0104, 65},
    {"priority", 4, 4, 0x0008, 2410},
    {"sampleOffset", 8, 4, 0x0008, 2411},
};

inline constexpr Field kFields_0044[] = {
    {"Layer", 0, 1, 0x0104, 66},
    {"sizeX", 4, 4, 0x0008, 2412},
    {"sizeY", 8, 4, 0x0008, 2413},
    {"sizeZ", 12, 4, 0x0008, 2414},
    {"fadeFactor", 16, 4, 0x0008, 2415},
    {"priority", 20, 4, 0x0008, 2416},
    {"Filename", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_0045[] = {
    {"Layer", 0, 1, 0x0104, 67},
    {"sizeX", 4, 4, 0x0008, 2417},
    {"sizeY", 8, 4, 0x0008, 2418},
    {"sizeZ", 12, 4, 0x0008, 2419},
    {"bias", 16, 4, 0x0008, 2420},
    {"blur", 20, 4, 0x0008, 2421},
    {"Filename", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_0046[] = {
    {"Filename", 0, 8, 0x0018, 0},
    {"FlowMapName", 8, 8, 0x0018, 0},
    {"CustomRTSwizzle", 16, 4, 0x0000, 97},
    {"Width", 20, 4, 0x0008, 2422},
    {"Height", 24, 4, 0x0008, 2423},
    {"Depth", 28, 4, 0x0008, 2424},
    {"FogStartDistance", 32, 4, 0x0008, 2425},
    {"GlobalDensityMultiplier", 36, 4, 0x0008, 2426},
    {"LocalDensity", 40, 4, 0x0008, 2427},
    {"LocalVerticalFalloff", 44, 4, 0x0008, 2428},
    {"FlowMapPeriod", 48, 4, 0x0008, 2429},
    {"FlowAmount", 52, 4, 0x0008, 2430},
    {"FogHeightOffset", 56, 4, 0x0008, 2431},
    {"FogHeightScale", 60, 4, 0x0008, 2432},
    {"FogFadeStartDistance", 64, 4, 0x0008, 2433},
    {"FogFadeEndDistance", 68, 4, 0x0008, 2434},
    {"Layer", 72, 1, 0x0104, 68},
    {"IsGlobalMap", 73, 1, 0x0014, 560},
};

inline constexpr Field kFields_0047[] = {
    {"Height", 0, 4, 0x0008, 2435},
    {"Radius", 4, 4, 0x0008, 2436},
};

inline constexpr Field kFields_0048[] = {
    {"Near", 80, 4, 0x0008, 2451},
    {"Far", 84, 4, 0x0008, 2452},
};

inline constexpr Field kFields_0049[] = {
    {"SnapTarget", 0, 4, 0x0000, 100},
};

inline constexpr Field kFields_004A[] = {
    {"MinCutoff", 0, 4, 0x0008, 2453},
    {"MinActive", 4, 4, 0x0008, 2454},
    {"MaxActive", 8, 4, 0x0008, 2455},
    {"MaxCutoff", 12, 4, 0x0008, 2456},
};

inline constexpr Field kFields_004B[] = {
    {"AssistDuration", 16, 4, 0x0008, 2461},
};

inline constexpr Field kFields_004C[] = {
    {"ActivationRange", 0, 4, 0x0008, 2462},
    {"MaxRange", 4, 4, 0x0008, 2463},
};

inline constexpr Field kFields_004D[] = {
    {"TweenIn", 0, 0, 0x002C, 12},
    {"TweenOut", 24, 0, 0x002C, 12},
    {"Preset", 48, 8, 0x0018, 0},
    {"Name", 56, 8, 0x0018, 0},
    {"Set", 64, 8, 0x0010, 0},
    {"Joint", 72, 8, 0x0010, 0},
    {"ZoomSnapRange", 80, 8, 0x001C, 75},
    {"LockOnRange", 88, 8, 0x001C, 76},
    {"Offset", 96, 0, 0x002C, 6},
    {"Priority", 102, 2, 0x0000, 101},
    {"CapsuleRatio", 104, 4, 0x0008, 2479},
    {"Type", 108, 4, 0x0204, 71},
    {"BaseWeight", 112, 4, 0x0008, 2480},
    {"MinDistance", 116, 4, 0x0008, 2481},
    {"MaxDistance", 120, 4, 0x0008, 2482},
    {"Radius", 124, 4, 0x0008, 2483},
    {"VisibilityRadius", 128, 4, 0x0008, 2484},
    {"Damping", 132, 4, 0x0008, 2485},
    {"DampingRadius", 136, 4, 0x0008, 2486},
    {"YawDamping", 140, 4, 0x0008, 2487},
    {"YawDampingMax", 144, 4, 0x0008, 2488},
    {"YawDampingSoftRatio", 148, 4, 0x0008, 2489},
    {"PitchDamping", 152, 4, 0x0008, 2490},
    {"PitchDampingMax", 156, 4, 0x0008, 2491},
    {"PitchDampingSoftRatio", 160, 4, 0x0008, 2492},
    {"RollDamping", 164, 4, 0x0008, 2493},
    {"RollDampingMax", 168, 4, 0x0008, 2494},
    {"RollDampingSoftRatio", 172, 4, 0x0008, 2495},
    {"ZoomSnapTargetRadius", 176, 4, 0x0008, 2496},
    {"ZoomSnapRadius", 180, 4, 0x0008, 2497},
    {"ZoomSnapScreenAngle", 184, 4, 0x0008, 2498},
    {"AimSphereCoreRadius", 188, 4, 0x0008, 2499},
    {"AimSphereRadius", 192, 4, 0x0008, 2500},
    {"AimSphereScreenAngle", 196, 4, 0x0008, 2501},
    {"AimCylinderCoreRadius", 200, 4, 0x0008, 2502},
    {"AimCylinderNearRadius", 204, 4, 0x0008, 2503},
    {"AimCylinderMidRadius", 208, 4, 0x0008, 2504},
    {"AimCylinderFarRadius", 212, 4, 0x0008, 2505},
    {"RadiusFromCapsule", 216, 1, 0x0014, 562},
    {"Synthetic", 217, 1, 0x0104, 72},
    {"DefaultOn", 218, 1, 0x0014, 563},
    {"SelfOcclude", 219, 1, 0x0014, 564},
    {"AlwaysVisible", 220, 1, 0x0014, 565},
    {"RespectYaw", 221, 1, 0x0014, 566},
    {"RespectPitch", 222, 1, 0x0014, 567},
    {"RespectRoll", 223, 1, 0x0014, 568},
};

inline constexpr Field kFields_004E[] = {
    {"Ratio", 0, 4, 0x0008, 2506},
    {"Minimum", 4, 4, 0x0008, 2507},
    {"Limit", 8, 4, 0x0008, 2508},
};

inline constexpr Field kFields_004F[] = {
    {"TimeStart", 0, 4, 0x0008, 2509},
    {"TimeDuration", 4, 4, 0x0008, 2510},
    {"MoveStart", 8, 4, 0x0008, 2511},
    {"MoveDistance", 12, 4, 0x0008, 2512},
    {"TimeStartHorizontal", 16, 4, 0x0008, 2513},
    {"TimeDurationHorizontal", 20, 4, 0x0008, 2514},
    {"MoveStartHorizontal", 24, 4, 0x0008, 2515},
    {"MoveDistanceHorizontal", 28, 4, 0x0008, 2516},
    {"TimeStartVertical", 32, 4, 0x0008, 2517},
    {"TimeDurationVertical", 36, 4, 0x0008, 2518},
    {"MoveStartVertical", 40, 4, 0x0008, 2519},
    {"MoveDistanceVertical", 44, 4, 0x0008, 2520},
    {"AngularVelocity", 48, 4, 0x0008, 2521},
    {"AngularVelocityMin", 52, 4, 0x0008, 2522},
    {"AngularVelocityMax", 56, 4, 0x0008, 2523},
    {"EaseIn", 60, 4, 0x0008, 2524},
    {"EaseOut", 64, 4, 0x0008, 2525},
    {"LengthIn", 68, 4, 0x0008, 2526},
    {"LengthOut", 72, 4, 0x0008, 2527},
    {"YawRange", 76, 4, 0x0008, 2528},
    {"PitchRange", 80, 4, 0x0008, 2529},
    {"TriggerRight", 84, 4, 0x0008, 2530},
    {"TriggerLeft", 88, 4, 0x0008, 2531},
    {"TriggerUp", 92, 4, 0x0008, 2532},
    {"TriggerDown", 96, 4, 0x0008, 2533},
    {"ReturnRight", 100, 4, 0x0008, 2534},
    {"ReturnLeft", 104, 4, 0x0008, 2535},
    {"ReturnUp", 108, 4, 0x0008, 2536},
    {"ReturnDown", 112, 4, 0x0008, 2537},
    {"StickCancelZoneHorizontal", 116, 4, 0x0008, 2538},
    {"StickCancelZoneVertical", 120, 4, 0x0008, 2539},
    {"RotationSoft", 124, 0, 0x002C, 78},
    {"ElevationSoft", 136, 0, 0x002C, 78},
    {"RotationSpace", 148, 1, 0x0104, 73},
    {"Direction", 149, 1, 0x0104, 74},
};

inline constexpr Field kFields_0050[] = {
    {"RotationSpeed", 80, 4, 0x0008, 2560},
    {"RotationMaxThreshold", 84, 4, 0x0008, 2561},
    {"RotationMaxVelocity", 88, 4, 0x0008, 2562},
    {"RotationAcceleration", 92, 4, 0x0008, 2563},
    {"RotationDecay", 96, 4, 0x0008, 2564},
    {"RotationSoftConstraintRatio", 100, 4, 0x0008, 2565},
    {"RotationAimFriction", 104, 4, 0x0008, 2566},
    {"RotationStrafeAssist", 108, 4, 0x0008, 2567},
    {"ElevationSpeed", 112, 4, 0x0008, 2568},
    {"ElevationMaxThreshold", 116, 4, 0x0008, 2569},
    {"ElevationMaxVelocity", 120, 4, 0x0008, 2570},
    {"ElevationAcceleration", 124, 4, 0x0008, 2571},
    {"ElevationDecay", 128, 4, 0x0008, 2572},
    {"ElevationSoftConstraintRatio", 132, 4, 0x0008, 2573},
    {"ElevationAimFriction", 136, 4, 0x0008, 2574},
    {"ElevationStrafeAssist", 140, 4, 0x0008, 2575},
    {"ControlType", 144, 1, 0x0104, 77},
};

inline constexpr Field kFields_0051[] = {
    {"VelocityMin", 0, 4, 0x0008, 2576},
    {"VelocityMax", 4, 4, 0x0008, 2577},
    {"AngleForwardMin", 8, 4, 0x0008, 2578},
    {"AngleForwardMax", 12, 4, 0x0008, 2579},
    {"AngleBackwardMin", 16, 4, 0x0008, 2580},
    {"AngleBackwardMax", 20, 4, 0x0008, 2581},
    {"AngleBackwardClamp", 24, 4, 0x0008, 2582},
    {"FilterSpace", 28, 1, 0x0104, 78},
};

inline constexpr Field kFields_0052[] = {
    {"RotationSpace", 0, 1, 0x0104, 79},
    {"RotationMax", 4, 4, 0x0008, 2583},
    {"RotationLeft", 8, 4, 0x0008, 2584},
    {"RotationRight", 12, 4, 0x0008, 2585},
    {"RotationSoft", 16, 0, 0x002C, 78},
    {"ElevationMax", 28, 4, 0x0008, 2589},
    {"ElevationMaxUp", 32, 4, 0x0008, 2590},
    {"ElevationMaxDown", 36, 4, 0x0008, 2591},
    {"ElevationSoft", 40, 0, 0x002C, 78},
};

inline constexpr Field kFields_0053[] = {
    {"From", 0, 12, 0x0024, 14},
    {"To", 16, 12, 0x0024, 15},
    {"Tween", 28, 0, 0x002C, 12},
    {"IgnoreIf", 52, 1, 0x0014, 570},
    {"OnlyIf", 53, 1, 0x0014, 571},
    {"Effect", 56, 12, 0x0024, 16},
};

inline constexpr Field kFields_0055[] = {
    {"Top", 0, 4, 0x0008, 2604},
    {"Bottom", 4, 4, 0x0008, 2605},
    {"Left", 8, 4, 0x0008, 2606},
    {"Right", 12, 4, 0x0008, 2607},
};

inline constexpr Field kFields_0056[] = {
    {"Left", 0, 4, 0x0008, 2608},
    {"Right", 4, 4, 0x0008, 2609},
    {"Center", 8, 4, 0x0008, 2610},
    {"RampIn", 12, 4, 0x0008, 2611},
    {"MinStrafe", 16, 4, 0x0008, 2612},
    {"MaxStrafe", 20, 4, 0x0008, 2613},
    {"MinNear", 24, 4, 0x0008, 2614},
    {"MaxRotateSpeed", 28, 4, 0x0008, 2615},
};

inline constexpr Field kFields_0057[] = {
    {"Key", 0, 12, 0x0024, 17},
    {"InputTarget", 12, 1, 0x0104, 80},
};

inline constexpr Field kFields_0058[] = {
    {"Curve", 0, 8, 0x0018, 0},
    {"SnapToNearest", 8, 1, 0x0014, 572},
    {"FlipCurve", 9, 1, 0x0014, 573},
    {"MinDistance", 12, 4, 0x0008, 2616},
    {"MaxDistance", 16, 4, 0x0008, 2617},
    {"CurveAdvance", 20, 4, 0x0008, 2618},
};

inline constexpr Field kFields_0059[] = {
    {"Position", 224, 4, 0x0008, 2661},
    {"StartTarget", 232, 8, 0x0018, 0},
    {"StartOffset", 240, 0, 0x002C, 7},
    {"StartStrength", 252, 4, 0x0008, 2665},
    {"EndTarget", 256, 8, 0x0018, 0},
    {"EndOffset", 264, 0, 0x002C, 7},
    {"EndStrength", 276, 4, 0x0008, 2669},
};

inline constexpr Field kFields_005A[] = {
    {"SafeZone", 80, 0, 0x002C, 85},
    {"PlayerSafeZone", 96, 0, 0x002C, 85},
    {"PlayerFrame", 112, 0, 0x002C, 85},
    {"TargetFrame", 128, 0, 0x002C, 85},
    {"TiltFrame", 144, 0, 0x002C, 85},
    {"Position", 160, 0, 0x002C, 84},
    {"Yaw", 172, 4, 0x0008, 2707},
    {"RequireMarker", 176, 12, 0x0024, 18},
    {"Pitch", 188, 4, 0x0008, 2708},
    {"IgnoreMarker", 192, 12, 0x0024, 19},
    {"Roll", 204, 4, 0x0008, 2709},
    {"TweenOverrides", 208, 12, 0x0024, 20},
    {"focalLength", 220, 4, 0x0008, 2710},
    {"Transition", 224, 12, 0x0024, 21},
    {"AngleOfView", 236, 4, 0x0008, 2711},
    {"Effect", 240, 12, 0x0024, 22},
    {"StaticTarget", 252, 0, 0x002C, 7},
    {"TemplateSymbol", 264, 8, 0x001A, 0},
    {"SplineTarget", 272, 8, 0x001C, 89},
    {"TargetMatches", 280, 8, 0x0018, 0},
    {"StrafeAssistTargetMatches", 288, 8, 0x0018, 0},
    {"TargetActivatedMatches", 296, 8, 0x0018, 0},
    {"RotateToTargetMatches", 304, 8, 0x0018, 0},
    {"TiltTargetMatches", 312, 8, 0x0018, 0},
    {"OrbitControl", 320, 8, 0x001C, 80},
    {"TiltControl", 328, 8, 0x001C, 80},
    {"OrbitConstraint", 336, 8, 0x001C, 82},
    {"TiltConstraint", 344, 8, 0x001C, 82},
    {"Recenter", 352, 8, 0x001C, 79},
    {"AutoRecenter", 360, 8, 0x001C, 79},
    {"TiltRecenter", 368, 8, 0x001C, 79},
    {"Animation", 376, 8, 0x0018, 0},
    {"ObjectTarget", 384, 8, 0x0018, 1},
    {"OrbitTarget", 392, 8, 0x0018, 2},
    {"CollisionTarget", 400, 8, 0x0018, 3},
    {"Curve", 408, 8, 0x0018, 0},
    {"DriveRail", 416, 8, 0x0018, 0},
    {"Follow", 424, 8, 0x001C, 81},
    {"PitchAdjust", 432, 8, 0x001C, 87},
    {"StrafeAssistFrame", 440, 8, 0x001C, 86},
    {"RotateToRail", 448, 8, 0x001C, 88},
    {"FocusDistance", 456, 4, 0x0008, 2715},
    {"FStop", 460, 4, 0x0008, 2716},
    {"LensDistortion", 464, 4, 0x0008, 2717},
    {"OrthoWidth", 468, 4, 0x0008, 2718},
    {"DefaultTweenTime", 472, 4, 0x0008, 2719},
    {"DefaultTweenDistance", 476, 4, 0x0008, 2720},
    {"DefaultEaseIn", 480, 4, 0x0008, 2721},
    {"DefaultEaseOut", 484, 4, 0x0008, 2722},
    {"DefaultLengthIn", 488, 4, 0x0008, 2723},
    {"DefaultLengthOut", 492, 4, 0x0008, 2724},
    {"BoomDamping", 496, 4, 0x0008, 2725},
    {"VerticalDamping", 500, 4, 0x0008, 2726},
    {"HorizontalDamping", 504, 4, 0x0008, 2727},
    {"DampingForward", 508, 4, 0x0008, 2728},
    {"DampingBackward", 512, 4, 0x0008, 2729},
    {"DampingLeft", 516, 4, 0x0008, 2730},
    {"DampingRight", 520, 4, 0x0008, 2731},
    {"DampingUp", 524, 4, 0x0008, 2732},
    {"DampingDown", 528, 4, 0x0008, 2733},
    {"MinVelocity", 532, 4, 0x0008, 2734},
    {"MaxVelocity", 536, 4, 0x0008, 2735},
    {"VelocityEaseIn", 540, 4, 0x0008, 2736},
    {"VelocityEaseOut", 544, 4, 0x0008, 2737},
    {"VelocityDampingTime", 548, 4, 0x0008, 2738},
    {"AnimationRate", 552, 4, 0x0008, 2739},
    {"BoomRatio", 556, 4, 0x0008, 2740},
    {"MaxDistanceToDolly", 560, 4, 0x0008, 2741},
    {"MinDistanceToDolly", 564, 4, 0x0008, 2742},
    {"MaxDistanceToTarget", 568, 4, 0x0008, 2743},
    {"MinDistanceToTarget", 572, 4, 0x0008, 2744},
    {"ElevationConstraint", 576, 4, 0x0008, 2745},
    {"RotationConstraint", 580, 4, 0x0008, 2746},
    {"AngleOfViewConstraint", 584, 4, 0x0008, 2747},
    {"DollyDamping", 588, 4, 0x0008, 2748},
    {"JumpCompensationFactor", 592, 4, 0x0008, 2749},
    {"MicBoomRatio", 596, 4, 0x0008, 2750},
    {"DollyStartDefault", 600, 4, 0x0008, 2751},
    {"FightLineAngle", 604, 4, 0x0008, 2752},
    {"RailFadeShelf", 608, 4, 0x0008, 2753},
    {"RailFadeFalloff", 612, 4, 0x0008, 2754},
    {"RailFadeEaseIn", 616, 4, 0x0008, 2755},
    {"RailFadeEaseOut", 620, 4, 0x0008, 2756},
    {"LookConstraintUp", 624, 4, 0x0008, 2757},
    {"LookConstraintDown", 628, 4, 0x0008, 2758},
    {"LookConstraintLeft", 632, 4, 0x0008, 2759},
    {"LookConstraintRight", 636, 4, 0x0008, 2760},
    {"MoveConstraintUp", 640, 4, 0x0008, 2761},
    {"MoveConstraintDown", 644, 4, 0x0008, 2762},
    {"MoveConstraintLeft", 648, 4, 0x0008, 2763},
    {"MoveConstraintRight", 652, 4, 0x0008, 2764},
    {"ElevateToFrameMax", 656, 4, 0x0008, 2765},
    {"ElevateToFrameMin", 660, 4, 0x0008, 2766},
    {"TargetFrameDamping", 664, 4, 0x0008, 2767},
    {"SlopeFactor", 668, 4, 0x0008, 2768},
    {"SlopeDamping", 672, 4, 0x0008, 2769},
    {"GroundInfluence", 676, 4, 0x0008, 2770},
    {"PanLeft", 680, 4, 0x0008, 2771},
    {"PanRight", 684, 4, 0x0008, 2772},
    {"TiltUp", 688, 4, 0x0008, 2773},
    {"TiltDown", 692, 4, 0x0008, 2774},
    {"Distance", 696, 4, 0x0008, 2775},
    {"StateFilter", 700, 2, 0x0204, 85},
    {"TargetCap", 702, 2, 0x0000, 107},
    {"MinimumTargetPriority", 704, 2, 0x0000, 108},
    {"MinimumTargetActivatedPriority", 706, 2, 0x0000, 109},
    {"CenterTargets", 708, 2, 0x0000, 110},
    {"CameraType", 710, 1, 0x0104, 86},
    {"CameraType_IsNull", 711, 1, 0x0016, 582},
    {"ParentRelative", 712, 1, 0x0014, 583},
    {"ParentRelative_IsNull", 713, 1, 0x0016, 584},
    {"Aggression", 714, 1, 0x0014, 585},
    {"Aggression_IsNull", 715, 1, 0x0016, 586},
    {"TargetActivated", 716, 1, 0x0014, 587},
    {"TargetActivated_IsNull", 717, 1, 0x0016, 588},
    {"Collision", 718, 1, 0x0014, 589},
    {"Collision_IsNull", 719, 1, 0x0016, 590},
    {"AvoidEverything", 720, 1, 0x0014, 591},
    {"AvoidEverything_IsNull", 721, 1, 0x0016, 592},
    {"DontCheckpoint", 722, 1, 0x0014, 593},
    {"DontCheckpoint_IsNull", 723, 1, 0x0016, 594},
    {"StartCentered", 724, 1, 0x0014, 595},
    {"StartCentered_IsNull", 725, 1, 0x0016, 596},
    {"RegisterName", 726, 1, 0x0014, 597},
    {"RegisterName_IsNull", 727, 1, 0x0016, 598},
    {"PhotoSensitive", 728, 1, 0x0014, 599},
    {"PhotoSensitive_IsNull", 729, 1, 0x0016, 600},
    {"ConstraintFriction", 730, 1, 0x0000, 111},
    {"ConstraintFriction_IsNull", 731, 1, 0x0016, 601},
    {"Position_IsNull", 732, 1, 0x0016, 602},
    {"Yaw_IsNull", 733, 1, 0x0016, 603},
    {"Pitch_IsNull", 734, 1, 0x0016, 604},
    {"Roll_IsNull", 735, 1, 0x0016, 605},
    {"focalLength_IsNull", 736, 1, 0x0016, 606},
    {"AngleOfView_IsNull", 737, 1, 0x0016, 607},
    {"FocusDistance_IsNull", 738, 1, 0x0016, 608},
    {"FStop_IsNull", 739, 1, 0x0016, 609},
    {"LensDistortion_IsNull", 740, 1, 0x0016, 610},
    {"DepthOfField", 741, 1, 0x0014, 611},
    {"DepthOfField_IsNull", 742, 1, 0x0016, 612},
    {"IsOrthographic", 743, 1, 0x0014, 613},
    {"IsOrthographic_IsNull", 744, 1, 0x0016, 614},
    {"OrthoWidth_IsNull", 745, 1, 0x0016, 615},
    {"StateFilter_IsNull", 746, 1, 0x0016, 616},
    {"RequireMarker_IsNull", 747, 1, 0x0016, 617},
    {"IgnoreMarker_IsNull", 748, 1, 0x0016, 618},
    {"DefaultTweenTime_IsNull", 749, 1, 0x0016, 619},
    {"DefaultTweenDistance_IsNull", 750, 1, 0x0016, 620},
    {"DefaultEaseIn_IsNull", 751, 1, 0x0016, 621},
    {"DefaultEaseOut_IsNull", 752, 1, 0x0016, 622},
    {"DefaultLengthIn_IsNull", 753, 1, 0x0016, 623},
    {"DefaultLengthOut_IsNull", 754, 1, 0x0016, 624},
    {"TweenOverrides_IsNull", 755, 1, 0x0016, 625},
    {"Transition_IsNull", 756, 1, 0x0016, 626},
    {"SplineTarget_IsNull", 757, 1, 0x0016, 627},
    {"TargetCap_IsNull", 758, 1, 0x0016, 628},
    {"MinimumTargetPriority_IsNull", 759, 1, 0x0016, 629},
    {"MinimumTargetActivatedPriority_IsNull", 760, 1, 0x0016, 630},
    {"TargetMatches_IsNull", 761, 1, 0x0016, 631},
    {"StrafeAssistTargetMatches_IsNull", 762, 1, 0x0016, 632},
    {"TargetActivatedMatches_IsNull", 763, 1, 0x0016, 633},
    {"RotateToTargetMatches_IsNull", 764, 1, 0x0016, 634},
    {"TiltTargetMatches_IsNull", 765, 1, 0x0016, 635},
    {"TargetFilter", 766, 1, 0x0204, 87},
    {"TargetFilter_IsNull", 767, 1, 0x0016, 636},
    {"BoomDamping_IsNull", 768, 1, 0x0016, 637},
    {"VerticalDamping_IsNull", 769, 1, 0x0016, 638},
    {"HorizontalDamping_IsNull", 770, 1, 0x0016, 639},
    {"DampingForward_IsNull", 771, 1, 0x0016, 640},
    {"DampingBackward_IsNull", 772, 1, 0x0016, 641},
    {"DampingLeft_IsNull", 773, 1, 0x0016, 642},
    {"DampingRight_IsNull", 774, 1, 0x0016, 643},
    {"DampingUp_IsNull", 775, 1, 0x0016, 644},
    {"DampingDown_IsNull", 776, 1, 0x0016, 645},
    {"MinVelocity_IsNull", 777, 1, 0x0016, 646},
    {"MaxVelocity_IsNull", 778, 1, 0x0016, 647},
    {"VelocityEaseIn_IsNull", 779, 1, 0x0016, 648},
    {"VelocityEaseOut_IsNull", 780, 1, 0x0016, 649},
    {"VelocityDampingTime_IsNull", 781, 1, 0x0016, 650},
    {"Effect_IsNull", 782, 1, 0x0016, 651},
    {"OrbitControl_IsNull", 783, 1, 0x0016, 652},
    {"TiltControl_IsNull", 784, 1, 0x0016, 653},
    {"OrbitConstraint_IsNull", 785, 1, 0x0016, 654},
    {"TiltConstraint_IsNull", 786, 1, 0x0016, 655},
    {"Recenter_IsNull", 787, 1, 0x0016, 656},
    {"AutoRecenter_IsNull", 788, 1, 0x0016, 657},
    {"TiltRecenter_IsNull", 789, 1, 0x0016, 658},
    {"Animation_IsNull", 790, 1, 0x0016, 659},
    {"AnimationRate_IsNull", 791, 1, 0x0016, 660},
    {"TriggerAnimation", 792, 1, 0x0014, 661},
    {"TriggerAnimation_IsNull", 793, 1, 0x0016, 662},
    {"ForceAnimation", 794, 1, 0x0014, 663},
    {"ForceAnimation_IsNull", 795, 1, 0x0016, 664},
    {"BoomRatio_IsNull", 796, 1, 0x0016, 665},
    {"MaxDistanceToDolly_IsNull", 797, 1, 0x0016, 666},
    {"MinDistanceToDolly_IsNull", 798, 1, 0x0016, 667},
    {"MaxDistanceToTarget_IsNull", 799, 1, 0x0016, 668},
    {"MinDistanceToTarget_IsNull", 800, 1, 0x0016, 669},
    {"ElevationConstraint_IsNull", 801, 1, 0x0016, 670},
    {"RotationConstraint_IsNull", 802, 1, 0x0016, 671},
    {"AngleOfViewConstraint_IsNull", 803, 1, 0x0016, 672},
    {"SafeZone_IsNull", 804, 1, 0x0016, 673},
    {"PlayerSafeZone_IsNull", 805, 1, 0x0016, 674},
    {"TrackToFrameTargets", 806, 1, 0x0014, 675},
    {"TrackToFrameTargets_IsNull", 807, 1, 0x0016, 676},
    {"FreeLook", 808, 1, 0x0014, 677},
    {"FreeLook_IsNull", 809, 1, 0x0016, 678},
    {"MultipleTargets", 810, 1, 0x0014, 679},
    {"MultipleTargets_IsNull", 811, 1, 0x0016, 680},
    {"DollyObscuredBoss", 812, 1, 0x0014, 681},
    {"DollyObscuredBoss_IsNull", 813, 1, 0x0016, 682},
    {"DollyObscuredPlayer", 814, 1, 0x0014, 683},
    {"DollyObscuredPlayer_IsNull", 815, 1, 0x0016, 684},
    {"TrackToMinimumDistance", 816, 1, 0x0014, 685},
    {"TrackToMinimumDistance_IsNull", 817, 1, 0x0016, 686},
    {"RailDrivesAnimation", 818, 1, 0x0014, 687},
    {"RailDrivesAnimation_IsNull", 819, 1, 0x0016, 688},
    {"MoveDolly", 820, 1, 0x0014, 689},
    {"MoveDolly_IsNull", 821, 1, 0x0016, 690},
    {"RailRelative", 822, 1, 0x0014, 691},
    {"RailRelative_IsNull", 823, 1, 0x0016, 692},
    {"JumpCompensation", 824, 1, 0x0014, 693},
    {"JumpCompensation_IsNull", 825, 1, 0x0016, 694},
    {"DoubleJumpComp", 826, 1, 0x0014, 695},
    {"DoubleJumpComp_IsNull", 827, 1, 0x0016, 696},
    {"UseStaticTarget", 828, 1, 0x0014, 697},
    {"UseStaticTarget_IsNull", 829, 1, 0x0016, 698},
    {"IgnoreMicMaxDistance", 830, 1, 0x0014, 699},
    {"IgnoreMicMaxDistance_IsNull", 831, 1, 0x0016, 700},
    {"OverrideMicBoomRatio", 832, 1, 0x0014, 701},
    {"OverrideMicBoomRatio_IsNull", 833, 1, 0x0016, 702},
    {"MicIsUnderwater", 834, 1, 0x0014, 703},
    {"MicIsUnderwater_IsNull", 835, 1, 0x0016, 704},
    {"CamIsUnderwater", 836, 1, 0x0014, 705},
    {"CamIsUnderwater_IsNull", 837, 1, 0x0016, 706},
    {"UsePlayerSafeZone", 838, 1, 0x0014, 707},
    {"UsePlayerSafeZone_IsNull", 839, 1, 0x0016, 708},
    {"Elevation", 840, 1, 0x0104, 88},
    {"Elevation_IsNull", 841, 1, 0x0016, 709},
    {"Rotation", 842, 1, 0x0104, 89},
    {"Rotation_IsNull", 843, 1, 0x0016, 710},
    {"DollyDamping_IsNull", 844, 1, 0x0016, 711},
    {"JumpCompensationFactor_IsNull", 845, 1, 0x0016, 712},
    {"MicBoomRatio_IsNull", 846, 1, 0x0016, 713},
    {"DollyStartDefault_IsNull", 847, 1, 0x0016, 714},
    {"StaticTarget_IsNull", 848, 1, 0x0016, 715},
    {"ObjectTarget_IsNull", 849, 1, 0x0016, 716},
    {"OrbitTarget_IsNull", 850, 1, 0x0016, 717},
    {"CollisionTarget_IsNull", 851, 1, 0x0016, 718},
    {"FightLineAngle_IsNull", 852, 1, 0x0016, 719},
    {"CenterTargets_IsNull", 853, 1, 0x0016, 720},
    {"RailFadeDirection", 854, 1, 0x0000, 112},
    {"RailFadeDirection_IsNull", 855, 1, 0x0016, 721},
    {"RailFadeShelf_IsNull", 856, 1, 0x0016, 722},
    {"RailFadeFalloff_IsNull", 857, 1, 0x0016, 723},
    {"RailFadeEaseIn_IsNull", 858, 1, 0x0016, 724},
    {"RailFadeEaseOut_IsNull", 859, 1, 0x0016, 725},
    {"Curve_IsNull", 860, 1, 0x0016, 726},
    {"DriveRail_IsNull", 861, 1, 0x0016, 727},
    {"LeftStick", 862, 1, 0x0104, 90},
    {"LeftStick_IsNull", 863, 1, 0x0016, 728},
    {"RightStick", 864, 1, 0x0104, 91},
    {"RightStick_IsNull", 865, 1, 0x0016, 729},
    {"LookFlags", 866, 1, 0x0204, 92},
    {"LookFlags_IsNull", 867, 1, 0x0016, 730},
    {"LookConstraintUp_IsNull", 868, 1, 0x0016, 731},
    {"LookConstraintDown_IsNull", 869, 1, 0x0016, 732},
    {"LookConstraintLeft_IsNull", 870, 1, 0x0016, 733},
    {"LookConstraintRight_IsNull", 871, 1, 0x0016, 734},
    {"MoveConstraintUp_IsNull", 872, 1, 0x0016, 735},
    {"MoveConstraintDown_IsNull", 873, 1, 0x0016, 736},
    {"MoveConstraintLeft_IsNull", 874, 1, 0x0016, 737},
    {"MoveConstraintRight_IsNull", 875, 1, 0x0016, 738},
    {"ElevateToFrameMax_IsNull", 876, 1, 0x0016, 739},
    {"ElevateToFrameMin_IsNull", 877, 1, 0x0016, 740},
    {"PlayerFrame_IsNull", 878, 1, 0x0016, 741},
    {"TargetFrame_IsNull", 879, 1, 0x0016, 742},
    {"TargetFrameDamping_IsNull", 880, 1, 0x0016, 743},
    {"Follow_IsNull", 881, 1, 0x0016, 744},
    {"SlopeFactor_IsNull", 882, 1, 0x0016, 745},
    {"SlopeDamping_IsNull", 883, 1, 0x0016, 746},
    {"GroundInfluence_IsNull", 884, 1, 0x0016, 747},
    {"FrameTargets", 885, 1, 0x0014, 748},
    {"FrameTargets_IsNull", 886, 1, 0x0016, 749},
    {"RotateToTargets", 887, 1, 0x0014, 750},
    {"RotateToTargets_IsNull", 888, 1, 0x0016, 751},
    {"TiltFrame_IsNull", 889, 1, 0x0016, 752},
    {"TiltFrameMode", 890, 1, 0x0104, 93},
    {"TiltFrameMode_IsNull", 891, 1, 0x0016, 753},
    {"PanLeft_IsNull", 892, 1, 0x0016, 754},
    {"PanRight_IsNull", 893, 1, 0x0016, 755},
    {"TiltUp_IsNull", 894, 1, 0x0016, 756},
    {"TiltDown_IsNull", 895, 1, 0x0016, 757},
    {"Distance_IsNull", 896, 1, 0x0016, 758},
    {"PitchAdjust_IsNull", 897, 1, 0x0016, 759},
    {"StrafeAssistFrame_IsNull", 898, 1, 0x0016, 760},
    {"RotateToRail_IsNull", 899, 1, 0x0016, 761},
};

inline constexpr Field kFields_005B[] = {
    {"Type", 0, 8, 0x0010, 0},
};

inline constexpr Field kFields_005C[] = {
    {"BottomToAnkle", 0, 4, 0x0008, 2776},
    {"BackToAnkle", 4, 4, 0x0008, 2777},
    {"MiddleToAnkle", 8, 4, 0x0008, 2778},
    {"FrontToMiddle", 12, 4, 0x0008, 2779},
    {"InToAnkle", 16, 4, 0x0008, 2780},
    {"OutToAnkle", 20, 4, 0x0008, 2781},
    {"BottomToToe", 24, 4, 0x0008, 2782},
};

inline constexpr Field kFields_005D[] = {
    {"FrontToMiddle", 0, 4, 0x0008, 2783},
    {"BackToWrist", 4, 4, 0x0008, 2784},
    {"BottomToWrist", 8, 4, 0x0008, 2785},
    {"InToWrist", 12, 4, 0x0008, 2786},
    {"OutToWrist", 16, 4, 0x0008, 2787},
};

inline constexpr Field kFields_005E[] = {
    {"HikcFilePath", 0, 8, 0x0018, 0},
    {"IkConfigurationFilePath", 8, 8, 0x0018, 0},
    {"IkFeetProperties", 16, 0, 0x002C, 92},
    {"IkHandsProperties", 44, 0, 0x002C, 93},
};

inline constexpr Field kFields_005F[] = {
    {"FirstChildIDX", 0, 4, 0x0000, 113},
    {"NextSiblingIDX", 4, 4, 0x0000, 114},
    {"MoveName", 8, 8, 0x0018, 0},
    {"MoveId", 16, 8, 0x0018, 0},
    {"Callback", 24, 8, 0x0018, 0},
    {"Flags", 32, 8, 0x0018, 0},
    {"LinkPosition", 40, 0, 0x002C, 9},
    {"LeftHandEndPosition", 48, 0, 0x002C, 6},
    {"LeftHandEndRotation", 54, 0, 0x002C, 9},
    {"RightHandEndPosition", 62, 0, 0x002C, 6},
    {"RightHandEndRotation", 68, 0, 0x002C, 9},
};

inline constexpr Field kFields_0060[] = {
    {"TraversePathGraph", 0, 12, 0x0024, 23},
};

inline constexpr Field kFields_0061[] = {
    {"MoveName", 0, 8, 0x0018, 0},
    {"MoveId", 8, 8, 0x0018, 0},
    {"Callback", 16, 8, 0x0018, 0},
    {"Flags", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_0063[] = {
    {"PrimaryCharacter", 0, 1, 0x0104, 94},
};

inline constexpr Field kFields_0064[] = {
    {"curvept", 0, 0, 0x002C, 9},
    {"tangentpt", 8, 0, 0x002C, 9},
};

inline constexpr Field kFields_0065[] = {
    {"curvepts", 0, 12, 0x0024, 24},
    {"num_segs", 12, 4, 0x0000, 115},
    {"anchors", 16, 12, 0x0024, 25},
    {"splinetype", 28, 4, 0x0000, 116},
};

inline constexpr Field kFields_0066[] = {
    {"Type", 0, 1, 0x0104, 95},
    {"ZoneFlags", 1, 1, 0x0204, 96},
    {"StaticWillNotMoveOrTouchDynamicNavmesh", 2, 1, 0x0014, 762},
};

inline constexpr Field kFields_0067[] = {
    {"ShapeType", 0, 1, 0x0104, 97},
    {"BoxWidth", 4, 4, 0x0008, 2826},
    {"BoxHeight", 8, 4, 0x0008, 2827},
    {"BoxDepth", 12, 4, 0x0008, 2828},
    {"SphereRadius", 16, 4, 0x0008, 2829},
    {"CylinderRadius", 20, 4, 0x0008, 2830},
    {"CylinderHeight", 24, 4, 0x0008, 2831},
    {"SweepAngleStartDistance", 28, 4, 0x0008, 2832},
    {"SweepAngleEndDistance", 32, 4, 0x0008, 2833},
    {"SweepAngle", 36, 4, 0x0008, 2834},
    {"SweepAngleHeight", 40, 4, 0x0008, 2835},
};

inline constexpr Field kFields_0068[] = {
    {"ContextActionZoneData", 0, 0, 0x002C, 103},
    {"ExitAngle", 44, 0, 0x002C, 7},
    {"MoveType", 56, 8, 0x0018, 4},
    {"LockPosition", 64, 8, 0x0018, 5},
    {"ForwardCallback", 72, 8, 0x0018, 0},
    {"BackwardCallback", 80, 8, 0x0018, 0},
    {"ForwardEndCallback", 88, 8, 0x0018, 0},
    {"BackwardEndCallback", 96, 8, 0x0018, 0},
    {"Distance", 104, 4, 0x0008, 2849},
    {"Height", 108, 4, 0x0008, 2850},
    {"XOffset", 112, 4, 0x0008, 2851},
    {"StartTLWidth", 116, 4, 0x0008, 2852},
    {"EndTLWidth", 120, 4, 0x0008, 2853},
    {"ForcedLocomotionMaxSpeed", 124, 4, 0x0008, 2854},
    {"ForcedLocomotionAcceleration", 128, 4, 0x0008, 2855},
    {"ZiplineNumber", 132, 4, 0x0000, 117},
    {"HintDistance", 136, 4, 0x0008, 2856},
    {"SystemicDisable", 140, 1, 0x0014, 763},
    {"WillNotMoveOrTouchDynamicNavmesh", 141, 1, 0x0014, 764},
    {"IsConnectedToExternalSystem", 142, 1, 0x0014, 765},
};

inline constexpr Field kFields_0069[] = {
    {"Text", 0, 8, 0x0018, 0},
    {"MinDistance", 8, 4, 0x0008, 2857},
    {"MaxDistance", 12, 4, 0x0008, 2858},
};

inline constexpr Field kFields_006A[] = {
    {"WetnessTintColor", 0, 0, 0x002C, 1},
    {"WetnessHeight", 16, 4, 0x0008, 2863},
    {"WetnessTransitionDistance", 20, 4, 0x0008, 2864},
    {"WetnessAmount", 24, 4, 0x0008, 2865},
    {"WetnessDarkenScale", 28, 4, 0x0008, 2866},
    {"WetnessFogBlend", 32, 4, 0x0008, 2867},
    {"WetnessFogAbsorption", 36, 4, 0x0008, 2868},
    {"WetnessTintEnable", 40, 1, 0x0014, 766},
};

inline constexpr Field kFields_006B[] = {
    {"Material", 0, 8, 0x0018, 0},
    {"Angle", 8, 4, 0x0008, 2869},
    {"Angular_Transition", 12, 4, 0x0008, 2870},
    {"Opacity", 16, 4, 0x0008, 2871},
    {"FadeOutDistance", 20, 4, 0x0008, 2872},
    {"FadeTransitionDistance", 24, 4, 0x0008, 2873},
    {"Z_Order", 28, 1, 0x0000, 118},
    {"ScaleUvByEntityScale", 29, 1, 0x0014, 767},
    {"Type", 30, 1, 0x0104, 99},
    {"ViewID", 31, 1, 0x0104, 100},
};

inline constexpr Field kFields_006C[] = {
    {"RotationalMass", 0, 0, 0x002C, 7},
    {"InitialLinearVelocity", 12, 0, 0x002C, 7},
    {"InitialAngularVelocity", 24, 0, 0x002C, 7},
    {"Mass", 36, 4, 0x0008, 2883},
    {"Friction", 40, 4, 0x0008, 2884},
    {"Bounciness", 44, 4, 0x0008, 2885},
    {"LinearDamping", 48, 4, 0x0008, 2886},
    {"AngularDamping", 52, 4, 0x0008, 2887},
    {"FadeOutOnSettleTime", 56, 4, 0x0008, 2888},
    {"RotationalMassType", 60, 1, 0x0104, 101},
    {"Flags", 61, 1, 0x0204, 102},
    {"FadeOutOnSettle", 62, 1, 0x0014, 768},
    {"IsGameObjectDriver", 63, 1, 0x0014, 769},
    {"IgnoreImpulseFromConcussion", 64, 1, 0x0014, 770},
    {"IsRagdoll", 65, 1, 0x0015, 771},
};

inline constexpr Field kFields_006D[] = {
    {"RigidBodyId", 0, 2, 0x0005, 103},
};

inline constexpr Field kFields_006E[] = {
    {"sizeX", 4, 4, 0x0008, 2889},
    {"sizeY", 8, 4, 0x0008, 2890},
    {"sizeZ", 12, 4, 0x0008, 2891},
};

inline constexpr Field kFields_006F[] = {
    {"Flags", 2, 1, 0x0204, 106},
    {"radius", 4, 4, 0x0008, 2892},
};

inline constexpr Field kFields_0070[] = {
    {"height", 4, 4, 0x0008, 2893},
    {"radius", 8, 4, 0x0008, 2894},
};

inline constexpr Field kFields_0071[] = {
    {"height", 4, 4, 0x0008, 2895},
    {"radius", 8, 4, 0x0008, 2896},
};

inline constexpr Field kFields_0072[] = {
    {"Width", 0, 4, 0x0008, 2897},
    {"Height", 4, 4, 0x0008, 2898},
    {"Depth", 8, 4, 0x0008, 2899},
    {"Layer", 12, 1, 0x0000, 119},
    {"WidthDivisions", 13, 1, 0x0000, 120},
    {"HeightDivisions", 14, 1, 0x0000, 121},
    {"DepthDivisions", 15, 1, 0x0000, 122},
};

inline constexpr Field kFields_0073[] = {
    {"Radius", 0, 4, 0x0008, 2900},
    {"Strength", 4, 4, 0x0008, 2901},
    {"Height", 8, 4, 0x0008, 2902},
    {"RadiusOther", 12, 4, 0x0008, 2903},
    {"Shape", 16, 1, 0x0104, 109},
    {"Type", 17, 1, 0x0104, 110},
    {"Projected", 18, 1, 0x0014, 772},
};

inline constexpr Field kFields_0074[] = {
    {"Width", 0, 4, 0x0008, 2904},
    {"Height", 4, 4, 0x0008, 2905},
    {"Depth", 8, 4, 0x0008, 2906},
    {"Layer", 12, 1, 0x0000, 123},
    {"WidthDivisions", 13, 1, 0x0000, 124},
    {"HeightDivisions", 14, 1, 0x0000, 125},
    {"DepthDivisions", 15, 1, 0x0000, 126},
};

inline constexpr Field kFields_0075[] = {
    {"Volumes", 0, 12, 0x0024, 26},
    {"height", 12, 4, 0x0008, 2907},
    {"radius", 16, 4, 0x0008, 2908},
    {"FlightVolumeType", 20, 1, 0x0104, 111},
};

inline constexpr Field kFields_0076[] = {
    {"Parameter", 0, 12, 0x0024, 27},
    {"Type", 12, 2, 0x0104, 112},
    {"Policy", 14, 1, 0x0004, 113},
    {"JointID", 15, 1, 0x0004, 114},
    {"Target", 16, 12, 0x0024, 28},
    {"VolumeID", 28, 1, 0x0004, 115},
};

inline constexpr Field kFields_0077[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"GroundType", 8, 8, 0x0204, 116},
    {"CollisionEntityType", 16, 1, 0x0104, 117},
    {"CollidesWith", 20, 4, 0x0204, 118},
    {"ThrowableResponse", 24, 8, 0x0010, 6},
};

inline constexpr Field kFields_0078[] = {
    {"PhysicsWorld", 0, 1, 0x0104, 119},
    {"CollisionSharedProperties", 8, 8, 0x0010, 7},
    {"MaterialFX", 16, 8, 0x0010, 8},
    {"CollisionID", 24, 1, 0x0000, 127},
    {"CollisionSharedPropertiesIndex", 26, 2, 0x0004, 120},
    {"NavMeshType", 28, 2, 0x0204, 121},
    {"IsEntityTrigger", 30, 1, 0x0014, 773},
};

inline constexpr Field kFields_007A[] = {
    {"LowSpeedDensity", 0, 4, 0x0008, 2909},
    {"HighSpeedDensity", 4, 4, 0x0008, 2910},
    {"HighSpeed", 8, 4, 0x0008, 2911},
    {"WeightScale", 12, 4, 0x0008, 2912},
};

inline constexpr Field kFields_007B[] = {
    {"Geometry", 0, 8, 0x0018, 0},
    {"MayaPath", 8, 8, 0x0018, 0},
    {"BurstStart", 16, 4, 0x0008, 2913},
    {"BurstInterval", 20, 4, 0x0008, 2914},
    {"BurstIntervalRandom", 24, 4, 0x0008, 2915},
    {"SmoothPosition", 28, 4, 0x0008, 2916},
    {"SmoothOrientation", 32, 4, 0x0008, 2917},
    {"directionX", 36, 4, 0x0008, 2918},
    {"directionY", 40, 4, 0x0008, 2919},
    {"directionZ", 44, 4, 0x0008, 2920},
    {"spread", 48, 4, 0x0008, 2921},
    {"SpreadRandom", 52, 4, 0x0008, 2922},
    {"speed", 56, 4, 0x0008, 2923},
    {"speedRandom", 60, 4, 0x0008, 2924},
    {"minDistance", 64, 4, 0x0008, 2925},
    {"maxDistance", 68, 4, 0x0008, 2926},
    {"rate", 72, 4, 0x0008, 2927},
    {"tangentSpeed", 76, 4, 0x0008, 2928},
    {"normalSpeed", 80, 4, 0x0008, 2929},
    {"directionalSpeed", 84, 4, 0x0008, 2930},
    {"volumeOffsetX", 88, 4, 0x0008, 2931},
    {"volumeOffsetY", 92, 4, 0x0008, 2932},
    {"volumeOffsetZ", 96, 4, 0x0008, 2933},
    {"volumeSweep", 100, 4, 0x0008, 2934},
    {"sectionRadius", 104, 4, 0x0008, 2935},
    {"awayFromCenter", 108, 4, 0x0008, 2936},
    {"awayFromAxis", 112, 4, 0x0008, 2937},
    {"alongAxis", 116, 4, 0x0008, 2938},
    {"aroundAxis", 120, 4, 0x0008, 2939},
    {"randomDirection", 124, 4, 0x0008, 2940},
    {"EmissionModulationStartDistance", 128, 4, 0x0008, 2941},
    {"EmissionModulationStartValue", 132, 4, 0x0008, 2942},
    {"EmissionModulationEndDistance", 136, 4, 0x0008, 2943},
    {"EmissionModulationEndValue", 140, 4, 0x0008, 2944},
    {"InheritVelocityAmt", 144, 4, 0x0008, 2945},
    {"InheritVelocityClamp", 148, 4, 0x0008, 2946},
    {"RateModulate", 152, 4, 0x0008, 2947},
    {"RateRandom", 156, 4, 0x0008, 2948},
    {"ScaleRateBySpeed", 160, 4, 0x0008, 2949},
    {"ScaleRateBySize", 164, 4, 0x0008, 2950},
    {"ScaleRateByDistance", 168, 4, 0x0008, 2951},
    {"ScaleRateOffset", 172, 4, 0x0008, 2952},
    {"EmitBySpeedThreshold", 176, 4, 0x0008, 2953},
    {"EmitByDistanceThreshold", 180, 4, 0x0008, 2954},
    {"EmitBySpeedInverseThreshold", 184, 4, 0x0008, 2955},
    {"ScriptedSpeed", 188, 4, 0x0008, 2956},
    {"BurstCount", 192, 2, 0x0000, 128},
    {"BurstMinParticleCount", 194, 2, 0x0000, 129},
    {"BurstMaxParticleCount", 196, 2, 0x0000, 130},
    {"EmissionCap", 198, 2, 0x0000, 131},
    {"emitterType", 200, 1, 0x0104, 122},
    {"CustomType", 201, 1, 0x0104, 123},
    {"volumeShape", 202, 1, 0x0104, 124},
    {"SubFrameInterpolation", 203, 1, 0x0104, 125},
    {"EmitFromSurface", 204, 1, 0x0014, 774},
    {"UseDecayColor", 205, 1, 0x0014, 775},
    {"DecayColorFromEmissive", 206, 1, 0x0014, 776},
    {"DecayUseEmitterVelDirection", 207, 1, 0x0014, 777},
    {"Mute", 208, 1, 0x0014, 778},
};

inline constexpr Field kFields_007C[] = {
    {"Index", 0, 4, 0x0000, 132},
    {"Position", 4, 4, 0x0008, 2957},
    {"FloatValue", 8, 4, 0x0008, 2958},
    {"Interp", 12, 1, 0x0104, 126},
};

inline constexpr Field kFields_007D[] = {
    {"Entries", 0, 12, 0x0024, 29},
};

inline constexpr Field kFields_007E[] = {
    {"Index", 0, 4, 0x0000, 133},
    {"Position", 4, 4, 0x0008, 2959},
    {"Red", 8, 4, 0x0008, 2960},
    {"Green", 12, 4, 0x0008, 2961},
    {"Blue", 16, 4, 0x0008, 2962},
    {"Interp", 20, 1, 0x0104, 127},
};

inline constexpr Field kFields_007F[] = {
    {"Entries", 0, 12, 0x0024, 30},
};

inline constexpr Field kFields_0080[] = {
    {"FieldType", 0, 1, 0x0104, 128},
    {"magnitude", 4, 4, 0x0008, 2963},
    {"attenuation", 8, 4, 0x0008, 2964},
    {"directionX", 12, 4, 0x0008, 2965},
    {"directionY", 16, 4, 0x0008, 2966},
    {"directionZ", 20, 4, 0x0008, 2967},
    {"minDistance", 24, 4, 0x0008, 2968},
    {"maxDistance", 28, 4, 0x0008, 2969},
    {"radialType", 32, 4, 0x0008, 2970},
    {"frequency", 36, 4, 0x0008, 2971},
    {"phaseX", 40, 4, 0x0008, 2972},
    {"phaseY", 44, 4, 0x0008, 2973},
    {"phaseZ", 48, 4, 0x0008, 2974},
    {"axisX", 52, 4, 0x0008, 2975},
    {"axisY", 56, 4, 0x0008, 2976},
    {"axisZ", 60, 4, 0x0008, 2977},
    {"directionalSpeed", 64, 4, 0x0008, 2978},
    {"useMaxDistance", 68, 1, 0x0014, 779},
    {"useDirection", 69, 1, 0x0014, 780},
    {"MayaPath", 72, 8, 0x0018, 0},
};

inline constexpr Field kFields_0081[] = {
    {"DecalCollisionResponse", 0, 32, 0x0024, 31},
    {"MFXCollisionSwitches", 32, 24, 0x0024, 32},
    {"smLifeTime", 56, 0, 0x002C, 125},
    {"smColour1", 72, 0, 0x002C, 127},
    {"smEmissiveIntensity", 88, 0, 0x002C, 125},
    {"smDistortion", 104, 0, 0x002C, 125},
    {"smOpacity1", 120, 0, 0x002C, 125},
    {"smScale", 136, 0, 0x002C, 125},
    {"smVelocityScaleRate", 152, 0, 0x002C, 125},
    {"smTwist", 168, 0, 0x002C, 125},
    {"Emitters", 184, 12, 0x0024, 41},
    {"lifespan", 196, 4, 0x0008, 2979},
    {"Fields", 200, 12, 0x0024, 42},
    {"LifespanRandom", 212, 4, 0x0008, 2980},
    {"MiniModels", 216, 12, 0x0024, 43},
    {"AngleFadeRate", 228, 4, 0x0008, 2981},
    {"FXCollisionResponse", 232, 8, 0x0010, 0},
    {"Identifier", 240, 8, 0x0018, 0},
    {"MayaPath", 248, 8, 0x0018, 0},
    {"FogDensityStrength", 256, 4, 0x0008, 2982},
    {"Colour1Random", 260, 4, 0x0008, 2983},
    {"NormalMapIntensityStart", 264, 4, 0x0008, 2984},
    {"NormalMapIntensityEnd", 268, 4, 0x0008, 2985},
    {"NormalMapIntensityAlpha", 272, 4, 0x0008, 2986},
    {"NormalMapIntensityJitter", 276, 4, 0x0008, 2987},
    {"ParticleColorBlendStart", 280, 4, 0x0008, 2988},
    {"ParticleColorBlendEnd", 284, 4, 0x0008, 2989},
    {"ParticleColorBlendAlpha", 288, 4, 0x0008, 2990},
    {"ParticleColorBlendJitter", 292, 4, 0x0008, 2991},
    {"OpacityStart", 296, 4, 0x0008, 2992},
    {"OpacityEnd", 300, 4, 0x0008, 2993},
    {"OpacityAlpha", 304, 4, 0x0008, 2994},
    {"Opacity1Random", 308, 4, 0x0008, 2995},
    {"OpacityTexturesExponent", 312, 4, 0x0008, 2996},
    {"OpacityTexturesExponentEnd", 316, 4, 0x0008, 2997},
    {"OpacityTexturesExponentAlpha", 320, 4, 0x0008, 2998},
    {"OpacityTexturesExponentJitter", 324, 4, 0x0008, 2999},
    {"Size", 328, 4, 0x0008, 3000},
    {"ScaleEnd", 332, 4, 0x0008, 3001},
    {"ScaleAlpha", 336, 4, 0x0008, 3002},
    {"ScaleRandom", 340, 4, 0x0008, 3003},
    {"CameraScaleStartDistance", 344, 4, 0x0008, 3004},
    {"CameraScaleStartValue", 348, 4, 0x0008, 3005},
    {"CameraScaleEndDistance", 352, 4, 0x0008, 3006},
    {"CameraScaleEndValue", 356, 4, 0x0008, 3007},
    {"VelocityScale", 360, 4, 0x0008, 3008},
    {"VelocityScaleClamp", 364, 4, 0x0008, 3009},
    {"TwistSpeed", 368, 4, 0x0008, 3010},
    {"StaticTwist", 372, 4, 0x0008, 3011},
    {"TwistSpeedRandom", 376, 4, 0x0008, 3012},
    {"CameraOffsetX", 380, 4, 0x0008, 3013},
    {"CameraOffsetY", 384, 4, 0x0008, 3014},
    {"CameraOffsetZ", 388, 4, 0x0008, 3015},
    {"FlipbookCycle", 392, 4, 0x0008, 3016},
    {"NearCull", 396, 4, 0x0008, 3017},
    {"NearCullFalloff", 400, 4, 0x0008, 3018},
    {"FarCull", 404, 4, 0x0008, 3019},
    {"FarCullFalloff", 408, 4, 0x0008, 3020},
    {"GravityX", 412, 4, 0x0008, 3021},
    {"GravityY", 416, 4, 0x0008, 3022},
    {"GravityZ", 420, 4, 0x0008, 3023},
    {"GravityIntensityStart", 424, 4, 0x0008, 3024},
    {"GravityIntensityEnd", 428, 4, 0x0008, 3025},
    {"GravityIntensityAlpha", 432, 4, 0x0008, 3026},
    {"GravityIntensityJitter", 436, 4, 0x0008, 3027},
    {"TurbulenceFrequencyX", 440, 4, 0x0008, 3028},
    {"TurbulenceFrequencyY", 444, 4, 0x0008, 3029},
    {"TurbulenceFrequencyZ", 448, 4, 0x0008, 3030},
    {"TurbulenceAmplitudeX", 452, 4, 0x0008, 3031},
    {"TurbulenceAmplitudeY", 456, 4, 0x0008, 3032},
    {"TurbulenceAmplitudeZ", 460, 4, 0x0008, 3033},
    {"TurbulencePhaseX", 464, 4, 0x0008, 3034},
    {"TurbulencePhaseY", 468, 4, 0x0008, 3035},
    {"TurbulencePhaseZ", 472, 4, 0x0008, 3036},
    {"TurbulenceScrollSpeedX", 476, 4, 0x0008, 3037},
    {"TurbulenceScrollSpeedY", 480, 4, 0x0008, 3038},
    {"TurbulenceScrollSpeedZ", 484, 4, 0x0008, 3039},
    {"TurbulenceIntensityStart", 488, 4, 0x0008, 3040},
    {"TurbulenceIntensityEnd", 492, 4, 0x0008, 3041},
    {"TurbulenceIntensityAlpha", 496, 4, 0x0008, 3042},
    {"TurbulenceIntensityJitter", 500, 4, 0x0008, 3043},
    {"Drag", 504, 4, 0x0008, 3044},
    {"DragEnd", 508, 4, 0x0008, 3045},
    {"DragAlpha", 512, 4, 0x0008, 3046},
    {"DragJitter", 516, 4, 0x0008, 3047},
    {"WindInfluenceOverLifeTimeStart", 520, 4, 0x0008, 3048},
    {"WindInfluenceOverLifeTimeEnd", 524, 4, 0x0008, 3049},
    {"WindInfluenceOverLifeTimeAlpha", 528, 4, 0x0008, 3050},
    {"WindInfluenceOverLifeTimeRandom", 532, 4, 0x0008, 3051},
    {"ConstrainMinDistance", 536, 4, 0x0008, 3052},
    {"ConstrainMaxDistance", 540, 4, 0x0008, 3053},
    {"SpeedModulate", 544, 4, 0x0008, 3054},
    {"SpriteCenterX", 548, 4, 0x0008, 3055},
    {"SpriteCenterY", 552, 4, 0x0008, 3056},
    {"SpriteCenterZ", 556, 4, 0x0008, 3057},
    {"SpriteWidth", 560, 4, 0x0008, 3058},
    {"SpriteHeight", 564, 4, 0x0008, 3059},
    {"SpriteDepth", 568, 4, 0x0008, 3060},
    {"SpriteWidthRandom", 572, 4, 0x0008, 3061},
    {"SpriteHeightRandom", 576, 4, 0x0008, 3062},
    {"SpriteDepthRandom", 580, 4, 0x0008, 3063},
    {"SpriteCameraOffset", 584, 4, 0x0008, 3064},
    {"SpriteRotation", 588, 4, 0x0008, 3065},
    {"UVScaleU", 592, 4, 0x0008, 3066},
    {"UVScaleV", 596, 4, 0x0008, 3067},
    {"UVOffsetU", 600, 4, 0x0008, 3068},
    {"UVOffsetV", 604, 4, 0x0008, 3069},
    {"UVScrollU", 608, 4, 0x0008, 3070},
    {"UVScrollV", 612, 4, 0x0008, 3071},
    {"SoftDistance", 616, 4, 0x0008, 3072},
    {"SoftDistanceInverse", 620, 4, 0x0008, 3073},
    {"MFXCollisionProbability", 624, 4, 0x0008, 3074},
    {"CollisionDecalSizeMin", 628, 4, 0x0008, 3075},
    {"CollisionDecalSizeMax", 632, 4, 0x0008, 3076},
    {"CollisionDecalDuration", 636, 4, 0x0008, 3077},
    {"CollisionBounciness", 640, 4, 0x0008, 3078},
    {"CollisionBouncinessRandom", 644, 4, 0x0008, 3079},
    {"CollisionRadius", 648, 4, 0x0008, 3080},
    {"DepthCollisionAgeThreshold", 652, 4, 0x0008, 3081},
    {"DepthCollisionThreshold", 656, 4, 0x0008, 3082},
    {"PlaneCollisionHeight", 660, 4, 0x0008, 3083},
    {"MaxCountClamp", 664, 2, 0x0000, 134},
    {"PerParticleLifeSpan", 666, 1, 0x0014, 781},
    {"UseLifeTimeRamp", 667, 1, 0x0014, 782},
    {"RenderType", 668, 1, 0x0104, 129},
    {"AlignMode", 669, 1, 0x0104, 130},
    {"SecondaryAlignMode", 670, 1, 0x0104, 131},
    {"SortMode", 671, 1, 0x0104, 132},
    {"ViewID", 672, 1, 0x0104, 133},
    {"AngleFade", 673, 1, 0x0014, 783},
    {"NormalMapIntensityType", 674, 1, 0x0104, 134},
    {"ParticleColorBlendType", 675, 1, 0x0104, 135},
    {"OpacityType", 676, 1, 0x0104, 136},
    {"OpacityTexturesExponentType", 677, 1, 0x0104, 137},
    {"ScaleType", 678, 1, 0x0104, 138},
    {"IgnoreScale", 679, 1, 0x0014, 784},
    {"RandomInitialTwist", 680, 1, 0x0014, 785},
    {"TwistDirection", 681, 1, 0x0104, 139},
    {"Layer", 682, 1, 0x0000, 135},
    {"LocalLayer", 683, 1, 0x0000, 136},
    {"SortSource", 684, 1, 0x0104, 140},
    {"BlendMode", 685, 1, 0x0104, 141},
    {"UseColorization", 686, 1, 0x0014, 786},
    {"SoftAlphaFadeOut", 687, 1, 0x0014, 787},
    {"RibbonOpacityRemap", 688, 1, 0x0014, 788},
    {"ForceRibbonEndFade", 689, 1, 0x0014, 789},
    {"ParticlesFollowSystem", 690, 1, 0x0014, 790},
    {"InheritEmitterVelocity", 691, 1, 0x0014, 791},
    {"ParticlesInCameraSpace", 692, 1, 0x0014, 792},
    {"FlipbookRandomStart", 693, 1, 0x0014, 793},
    {"GravityIntensityType", 694, 1, 0x0104, 142},
    {"TurbulenceIntensityType", 695, 1, 0x0104, 143},
    {"DragType", 696, 1, 0x0104, 144},
    {"WindInfluenceOverLifeTimeType", 697, 1, 0x0104, 145},
    {"ChangeConstraintDistanceByScale", 698, 1, 0x0014, 794},
    {"SynchToAnimationSpeed", 699, 1, 0x0014, 795},
    {"RandomUVFlipU", 700, 1, 0x0014, 796},
    {"RandomUVFlipV", 701, 1, 0x0014, 797},
    {"RibbonUVMode", 702, 1, 0x0104, 146},
    {"TessellateRibbonWidth", 703, 1, 0x0014, 798},
    {"MFXCollideWithEnvironment", 704, 1, 0x0014, 799},
    {"MFXCollideWithCharacters", 705, 1, 0x0014, 800},
    {"CollisionResponseOrientation", 706, 1, 0x0104, 147},
    {"PlaneCollision", 707, 1, 0x0014, 801},
    {"HeightFieldCollision", 708, 1, 0x0014, 802},
    {"ScaleRadiusWithParticleSize", 709, 1, 0x0014, 803},
    {"CancelVelocityX", 710, 1, 0x0014, 804},
    {"CancelVelocityY", 711, 1, 0x0014, 805},
    {"CancelVelocityZ", 712, 1, 0x0014, 806},
    {"ParticlesWrapAround", 713, 1, 0x0014, 807},
    {"ModulateEmissionByFXBlender", 714, 1, 0x0014, 808},
    {"ModulateAlphaByFXBlender", 715, 1, 0x0014, 809},
    {"HasDecayMesh", 716, 1, 0x0014, 810},
    {"MinimodelsCastShadows", 717, 1, 0x0014, 811},
    {"HideInPhotoMode", 718, 1, 0x0014, 812},
};

inline constexpr Field kFields_0082[] = {
    {"RayLength", 0, 4, 0x0008, 3084},
    {"LODDistance", 4, 4, 0x0008, 3085},
    {"MFXSwitch0", 8, 8, 0x0010, 0},
    {"MFXSwitch1", 16, 8, 0x0010, 0},
    {"MFXSwitch2", 24, 8, 0x0010, 0},
    {"OverrideJointName", 32, 8, 0x0010, 0},
    {"UseOwnerGameObject", 40, 1, 0x0014, 813},
};

inline constexpr Field kFields_0083[] = {
    {"TargetColor0", 0, 0, 0x002C, 1},
    {"TargetColor1", 16, 0, 0x002C, 1},
    {"TargetColor2", 32, 0, 0x002C, 1},
    {"TargetColor3", 48, 0, 0x002C, 1},
    {"TargetColor4", 64, 0, 0x002C, 1},
    {"TargetColor5", 80, 0, 0x002C, 1},
    {"TargetColor6", 96, 0, 0x002C, 1},
    {"TargetColor7", 112, 0, 0x002C, 1},
    {"Filename", 128, 8, 0x0018, 0},
    {"CameraBorderBufferSize", 136, 4, 0x0008, 3118},
    {"ClearFade", 140, 4, 0x0008, 3119},
    {"TargetAlpha0", 144, 4, 0x0008, 3120},
    {"TargetAlpha1", 148, 4, 0x0008, 3121},
    {"TargetAlpha2", 152, 4, 0x0008, 3122},
    {"TargetAlpha3", 156, 4, 0x0008, 3123},
    {"TargetAlpha4", 160, 4, 0x0008, 3124},
    {"TargetAlpha5", 164, 4, 0x0008, 3125},
    {"TargetAlpha6", 168, 4, 0x0008, 3126},
    {"TargetAlpha7", 172, 4, 0x0008, 3127},
    {"SizeXws", 176, 4, 0x0008, 3128},
    {"SizeYws", 180, 4, 0x0008, 3129},
    {"SizeZws", 184, 4, 0x0008, 3130},
    {"Width", 188, 2, 0x0000, 137},
    {"Height", 190, 2, 0x0000, 138},
    {"Follow", 192, 1, 0x0104, 148},
    {"ClearMethod", 193, 1, 0x0104, 149},
    {"DepthTest", 194, 1, 0x0014, 814},
    {"DoubleBuffer", 195, 1, 0x0014, 815},
    {"SnapFollowToPixel", 196, 1, 0x0014, 816},
    {"GenerateMips", 197, 1, 0x0014, 817},
    {"GenerateMipsBorderMode", 198, 1, 0x0104, 150},
    {"SortID", 199, 1, 0x0000, 139},
    {"TargetCount", 200, 1, 0x0000, 140},
    {"TargetFormat0", 201, 1, 0x0104, 151},
    {"TargetFormat1", 202, 1, 0x0104, 152},
    {"TargetFormat2", 203, 1, 0x0104, 153},
    {"TargetFormat3", 204, 1, 0x0104, 154},
    {"TargetFormat4", 205, 1, 0x0104, 155},
    {"TargetFormat5", 206, 1, 0x0104, 156},
    {"TargetFormat6", 207, 1, 0x0104, 157},
    {"TargetFormat7", 208, 1, 0x0104, 158},
};

inline constexpr Field kFields_0084[] = {
    {"Enabled", 0, 1, 0x0014, 818},
    {"Radius", 4, 4, 0x0008, 3131},
};

inline constexpr Field kFields_0085[] = {
    {"InteractZoneTweak", 0, 8, 0x0010, 0},
};

inline constexpr Field kFields_0086[] = {
    {"CandidateSet", 0, 8, 0x0010, 0},
    {"MPIconJointName", 8, 8, 0x0010, 0},
    {"NormalPromptName", 16, 8, 0x0010, 9},
    {"UnavailablePromptName", 24, 8, 0x0010, 10},
    {"BlockedPromptName", 32, 8, 0x0010, 11},
    {"LockedPromptName", 40, 8, 0x0010, 12},
    {"HintPromptName", 48, 8, 0x0010, 13},
    {"SubIconName", 56, 8, 0x0010, 0},
    {"HintSubIconName", 64, 8, 0x0010, 0},
    {"InteractLerpObjectName", 72, 8, 0x0018, 0},
    {"CreatureRequestFilterName", 80, 8, 0x0010, 0},
    {"InteractTag_0", 88, 8, 0x0018, 0},
    {"InteractTag_1", 96, 8, 0x0018, 0},
    {"InteractTag_2", 104, 8, 0x0018, 0},
    {"InteractTag_3", 112, 8, 0x0018, 0},
    {"InteractTag_4", 120, 8, 0x0018, 0},
    {"InteractTag_5", 128, 8, 0x0018, 0},
    {"InteractTag_6", 136, 8, 0x0018, 0},
    {"InteractTag_7", 144, 8, 0x0018, 0},
    {"HintPromptXZDistance", 152, 4, 0x0008, 3132},
    {"HintPromptAngle", 156, 4, 0x0008, 3133},
    {"HintPromptYDistance", 160, 4, 0x0008, 3134},
    {"GlintXZDistance", 164, 4, 0x0008, 3135},
    {"GlintYDistance", 168, 4, 0x0008, 3136},
    {"GlintAngle", 172, 4, 0x0008, 3137},
    {"ShowPromptXZDistance", 176, 4, 0x0008, 3138},
    {"ShowPromptYDistance", 180, 4, 0x0008, 3139},
    {"YRangeMin", 184, 4, 0x0008, 3140},
    {"YRangeMax", 188, 4, 0x0008, 3141},
    {"XZRangeMin", 192, 4, 0x0008, 3142},
    {"XZRangeMax", 196, 4, 0x0008, 3143},
    {"PromptOffscreenDistance", 200, 4, 0x0008, 3144},
    {"InteractFrontAngleMin", 204, 4, 0x0008, 3145},
    {"InteractFrontAngleMax", 208, 4, 0x0008, 3146},
    {"PlayerFrontAngleMin", 212, 4, 0x0008, 3147},
    {"PlayerFrontAngleMax", 216, 4, 0x0008, 3148},
    {"CameraFrontAngleMin", 220, 4, 0x0008, 3149},
    {"CameraFrontAngleMax", 224, 4, 0x0008, 3150},
    {"InteractFrontPlayerFrontAngleMin", 228, 4, 0x0008, 3151},
    {"InteractFrontPlayerFrontAngleMax", 232, 4, 0x0008, 3152},
    {"OnScreenPercentMin", 236, 4, 0x0008, 3153},
    {"OnScreenPercentMax", 240, 4, 0x0008, 3154},
    {"YRangeWeight", 244, 4, 0x0008, 3155},
    {"XZRangeWeight", 248, 4, 0x0008, 3156},
    {"InteractFrontAngleWeight", 252, 4, 0x0008, 3157},
    {"PlayerFrontAngleWeight", 256, 4, 0x0008, 3158},
    {"CameraFrontAngleWeight", 260, 4, 0x0008, 3159},
    {"InteractFrontPlayerFrontAngleWeight", 264, 4, 0x0008, 3160},
    {"OnScreenPercentWeight", 268, 4, 0x0008, 3161},
    {"LineOfSightWeight", 272, 4, 0x0008, 3162},
    {"CurrentTargetWeight", 276, 4, 0x0008, 3163},
    {"CombatExtraWeight", 280, 4, 0x0008, 3164},
    {"Flags", 284, 2, 0x0204, 159},
    {"SystemToNotifyWhenTriggered", 286, 1, 0x0104, 160},
};

inline constexpr Field kFields_0087[] = {
    {"ScriptPath", 0, 8, 0x0018, 0},
};

inline constexpr Field kFields_0088[] = {
    {"Dummy", 0, 1, 0x0014, 819},
};

inline constexpr Field kFields_0089[] = {
    {"Dummy", 0, 1, 0x0014, 820},
};

inline constexpr Field kFields_008A[] = {
    {"LootType", 0, 1, 0x0104, 161},
};

inline constexpr Field kFields_008B[] = {
    {"Dummy", 0, 1, 0x0014, 821},
};

inline constexpr Field kFields_008C[] = {
    {"ConnectedTraverseLinkTypes", 0, 1, 0x0104, 162},
    {"ConnectedTraverseLinkTypes", 0, 12, 0x0024, 44},
    {"Type", 12, 1, 0x0104, 163},
    {"ConnectedTraverseLinkGUIDs", 16, 12, 0x0024, 45},
    {"BehaviorTreeFilePath", 32, 8, 0x0018, 0},
    {"BehaviorTreeName", 40, 8, 0x0018, 0},
    {"ConnectedTraverseGraphGUID", 48, 8, 0x0018, 0},
};

inline constexpr Field kFields_008D[] = {
    {"Segments", 0, 12, 0x0024, 46},
};

inline constexpr Field kFields_008E[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"Direction", 88, 0, 0x002C, 6},
    {"Direction_IsNull", 94, 1, 0x0016, 823},
    {"Strength", 96, 4, 0x0008, 3182},
    {"Strength_IsNull", 100, 1, 0x0016, 824},
    {"GustFactor", 104, 4, 0x0008, 3183},
    {"GustFactor_IsNull", 108, 1, 0x0016, 825},
};

inline constexpr Field kFields_008F[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"EmissionModulate", 88, 4, 0x0008, 3198},
    {"EmissionModulate_IsNull", 92, 1, 0x0016, 827},
    {"AlphaModulate", 96, 4, 0x0008, 3199},
    {"AlphaModulate_IsNull", 100, 1, 0x0016, 828},
};

inline constexpr Field kFields_0090[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"DepthSpawnThreshold", 88, 4, 0x0008, 3214},
    {"MinScale", 92, 4, 0x0008, 3215},
    {"MaxScale", 96, 4, 0x0008, 3216},
    {"PokingFactor", 100, 4, 0x0008, 3217},
    {"DensityByDisplacementDistance", 104, 4, 0x0008, 3218},
    {"DensityByDisplacementDistanceSpeed", 108, 4, 0x0008, 3219},
    {"ModelType", 112, 1, 0x0104, 170},
    {"ModelType_IsNull", 113, 1, 0x0016, 830},
    {"Density", 114, 1, 0x0000, 147},
    {"Density_IsNull", 115, 1, 0x0016, 831},
    {"SpawnAreaDistance", 116, 1, 0x0000, 148},
    {"SpawnAreaDistance_IsNull", 117, 1, 0x0016, 832},
    {"DepthSpawnThreshold_IsNull", 118, 1, 0x0016, 833},
    {"MinScale_IsNull", 119, 1, 0x0016, 834},
    {"MaxScale_IsNull", 120, 1, 0x0016, 835},
    {"PokingFactor_IsNull", 121, 1, 0x0016, 836},
    {"DensityByDisplacementDistance_IsNull", 122, 1, 0x0016, 837},
    {"DensityByDisplacementDistanceSpeed_IsNull", 123, 1, 0x0016, 838},
    {"DetailModelsEnabled", 124, 1, 0x0014, 839},
    {"DetailModelsEnabled_IsNull", 125, 1, 0x0016, 840},
};

inline constexpr Field kFields_0091[] = {
    {"CreatureCategory", 0, 1, 0x0104, 171},
    {"CameraDistanceScaleMultiplier", 4, 4, 0x0008, 3220},
    {"CameraDistanceOffset", 8, 4, 0x0008, 3221},
    {"LODOffset", 12, 4, 0x0000, 149},
};

inline constexpr Field kFields_0092[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"EnableOnPS5", 88, 1, 0x0014, 842},
    {"EnableOnPS5_IsNull", 89, 1, 0x0016, 843},
    {"EnableOnPS4", 90, 1, 0x0014, 844},
    {"EnableOnPS4_IsNull", 91, 1, 0x0016, 845},
    {"CreatureSettings", 92, 256, 0x0024, 47},
    {"CreatureSettings_IsNull", 348, 1, 0x0016, 846},
};

inline constexpr Field kFields_0093[] = {
    {"Weather0ID", 0, 8, 0x0010, 0},
    {"Weather1ID", 8, 8, 0x0010, 0},
    {"Weather2ID", 16, 8, 0x0010, 0},
    {"Duration", 24, 4, 0x0008, 3236},
    {"WadName", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_0094[] = {
    {"Execution", 0, 1, 0x0104, 174},
    {"WeatherStates", 8, 160, 0x0024, 48},
};

inline constexpr Field kFields_0095[] = {
    {"Text", 0, 8, 0x0018, 0},
    {"Style", 8, 8, 0x0018, 0},
    {"Camera", 16, 8, 0x0010, 0},
    {"MsgID", 24, 4, 0x0000, 152},
    {"Size", 28, 4, 0x0008, 3237},
    {"Layer", 32, 4, 0x0104, 175},
    {"Wrap", 36, 1, 0x0014, 847},
    {"DisableAlignmentFlip", 37, 1, 0x0014, 848},
    {"Pause", 38, 1, 0x0014, 849},
};

inline constexpr Field kFields_0096[] = {
    {"BoxObject", 0, 8, 0x0010, 0},
    {"TextObject", 8, 8, 0x0010, 0},
    {"NextTransform", 16, 8, 0x0010, 0},
    {"BoxHeight", 24, 4, 0x0008, 3238},
    {"TextHeight", 28, 4, 0x0008, 3239},
};

inline constexpr Field kFields_0098[] = {
    {"Allowed", 0, 2, 0x0000, 153},
    {"Rejected", 2, 2, 0x0000, 154},
};

inline constexpr Field kFields_0099[] = {
    {"Allowed", 0, 4, 0x0000, 155},
    {"Rejected", 4, 4, 0x0000, 156},
};

inline constexpr Field kFields_009A[] = {
    {"State", 0, 1, 0x0000, 157},
    {"StateHash", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_009B[] = {
    {"Group", 0, 8, 0x0010, 0},
    {"Flags", 8, 12, 0x0024, 49},
};

inline constexpr Field kFields_009C[] = {
    {"Type", 0, 1, 0x0104, 176},
};

inline constexpr Field kFields_009D[] = {
    {"AngularSpeed", 4, 4, 0x0008, 3240},
};

inline constexpr Field kFields_00A1[] = {
    {"TyresFront", 0, 0, 0x002C, 1433},
    {"TyresRear", 28, 0, 0x002C, 1433},
    {"SuspensionsFront", 56, 0, 0x002C, 1435},
    {"SuspensionsRear", 80, 0, 0x002C, 1435},
    {"Engine", 104, 0, 0x002C, 1436},
    {"FrontBrakeTorque", 124, 4, 0x0008, 3273},
    {"GearBox", 128, 0, 0x002C, 1439},
    {"Turbo", 144, 0, 0x002C, 1440},
    {"DifferentialFront", 160, 0, 0x002C, 1437},
    {"DifferentialCenter", 172, 0, 0x002C, 1437},
    {"DifferentialRear", 184, 0, 0x002C, 1437},
    {"ArcadeBrake", 196, 0, 0x002C, 1441},
    {"ActiveSteering", 208, 0, 0x002C, 1442},
    {"RearBrakeTorque", 220, 4, 0x0008, 3294},
    {"MaxSpeed", 224, 4, 0x0008, 3295},
    {"ArcadeHandbrakeGripLoss", 228, 4, 0x0008, 3296},
};

inline constexpr Field kFields_00A2[] = {
    {"SteerLimits", 0, 0, 0x002C, 1443},
    {"AutoSteer", 12, 0, 0x002C, 1444},
    {"SteerTime", 32, 4, 0x0008, 3305},
    {"SteerReleaseTime", 36, 4, 0x0008, 3306},
};

inline constexpr Field kFields_00A3[] = {
    {"SpeedForFullEffect", 0, 4, 0x0008, 3307},
    {"SpeedForNoEffect", 4, 4, 0x0008, 3308},
    {"BrakeAmountAtFullEffect", 8, 4, 0x0008, 3309},
};

inline constexpr Field kFields_00A4[] = {
    {"SteeringAngleFront", 0, 4, 0x0008, 3310},
    {"SteeringAngleRear", 4, 4, 0x0008, 3311},
    {"WheelRadiusFront", 8, 4, 0x0008, 3312},
    {"WheelRadiusRear", 12, 4, 0x0008, 3313},
    {"SuspensionMaxExtensionFront", 16, 4, 0x0008, 3314},
    {"SuspensionMaxExtensionRear", 20, 4, 0x0008, 3315},
    {"SuspensionRestExtensionFront", 24, 4, 0x0008, 3316},
    {"SuspensionRestExtensionRear", 28, 4, 0x0008, 3317},
    {"SuspensionToCOMFrontRight", 32, 0, 0x002C, 7},
    {"SuspensionToCOMRearRight", 44, 0, 0x002C, 7},
};

inline constexpr Field kFields_00A5[] = {
    {"MaxSpeedMultiplier", 0, 4, 0x0008, 3324},
    {"Responsiveness", 4, 4, 0x0008, 3325},
    {"ExtraDriftiness", 8, 4, 0x0008, 3326},
    {"LongitudinalGrip", 12, 4, 0x0008, 3327},
    {"LateralGrip", 16, 4, 0x0008, 3328},
    {"DecelerationOverMaxSpeed", 20, 4, 0x0008, 3329},
};

inline constexpr Field kFields_00A6[] = {
    {"Physics", 0, 0, 0x002C, 161},
    {"SteeringFilters", 232, 0, 0x002C, 162},
    {"AutoBrake", 272, 0, 0x002C, 163},
    {"MayaOverrides", 284, 0, 0x002C, 164},
    {"HandlingPerMaterial", 344, 0, 0x0028, 0},
};

inline constexpr Field kFields_00A7[] = {
    {"Chassis", 0, 8, 0x0010, 14},
    {"FrontLeftSuspensionJoint", 8, 8, 0x0010, 15},
    {"FrontRightSuspensionJoint", 16, 8, 0x0010, 16},
    {"RearLeftSuspensionJoint", 24, 8, 0x0010, 17},
    {"RearRightSuspensionJoint", 32, 8, 0x0010, 18},
    {"FrontLeftSteeringJoint", 40, 8, 0x0010, 19},
    {"FrontRightSteeringJoint", 48, 8, 0x0010, 20},
    {"RearLeftSteeringJoint", 56, 8, 0x0010, 21},
    {"RearRightSteeringJoint", 64, 8, 0x0010, 22},
    {"FrontLeftSuspensionSDKChannel", 72, 8, 0x0018, 23},
    {"FrontRightSuspensionSDKChannel", 80, 8, 0x0018, 24},
    {"RearLeftSuspensionSDKChannel", 88, 8, 0x0018, 25},
    {"RearRightSuspensionSDKChannel", 96, 8, 0x0018, 26},
    {"FrontLeftSteeringSDKChannel", 104, 8, 0x0018, 27},
    {"FrontRightSteeringSDKChannel", 112, 8, 0x0018, 28},
    {"RearLeftSteeringSDKChannel", 120, 8, 0x0018, 29},
    {"RearRightSteeringSDKChannel", 128, 8, 0x0018, 30},
    {"FrontLeftWheelRotationSDKChannel", 136, 8, 0x0018, 31},
    {"FrontRightWheelRotationSDKChannel", 144, 8, 0x0018, 32},
    {"RearLeftWheelRotationSDKChannel", 152, 8, 0x0018, 33},
    {"RearRightWheelRotationSDKChannel", 160, 8, 0x0018, 34},
};

inline constexpr Field kFields_00A8[] = {
    {"MaxAngle", 0, 4, 0x0008, 3412},
    {"MaxDistance", 4, 4, 0x0008, 3413},
    {"WeightAngle", 8, 4, 0x0008, 3414},
    {"WeightDistance", 12, 4, 0x0008, 3415},
    {"WeightOnScreen", 16, 4, 0x0008, 3416},
    {"MinScoreForLowRank", 20, 4, 0x0008, 3417},
    {"MinScoreForMediumRank", 24, 4, 0x0008, 3418},
    {"MinScoreForHighRank", 28, 4, 0x0008, 3419},
};

inline constexpr Field kFields_00A9[] = {
    {"IgnoreFlags", 0, 2, 0x0204, 180},
    {"ActiveFlags", 2, 2, 0x0204, 181},
};

inline constexpr Field kFields_00AA[] = {
    {"Type", 0, 1, 0x0105, 182},
    {"Flags", 1, 1, 0x0204, 183},
    {"Condition", 2, 1, 0x0104, 184},
    {"WeaponLevelMin", 3, 1, 0x0000, 158},
    {"On", 4, 2, 0x0008, 3420},
    {"Off", 6, 2, 0x0008, 3421},
};

inline constexpr Field kFields_00AB[] = {
    {"Wallet", 8, 8, 0x0010, 0},
    {"Resource", 16, 8, 0x0010, 0},
    {"Amount", 24, 4, 0x0000, 160},
};

inline constexpr Field kFields_00AC[] = {
    {"LootFlags", 8, 12, 0x0024, 52},
    {"ClearFlags", 20, 1, 0x0014, 850},
    {"Activate", 21, 1, 0x0014, 851},
};

inline constexpr Field kFields_00AD[] = {
    {"Wallet", 8, 8, 0x0010, 0},
    {"Roll", 16, 8, 0x0010, 0},
    {"IsConditionSet", 24, 1, 0x0014, 852},
};

inline constexpr Field kFields_00AE[] = {
    {"ActionList", 8, 12, 0x0024, 53},
    {"IgnoreFlags", 20, 0, 0x002C, 169},
    {"SkuList", 24, 12, 0x0024, 54},
    {"Chance", 36, 4, 0x0008, 3430},
    {"Decision", 40, 8, 0x001C, 1043},
    {"When", 48, 8, 0x0030, 65535},
    {"Else", 56, 8, 0x001C, 1445},
    {"Filter", 64, 1, 0x0204, 200},
};

inline constexpr Field kFields_00AF[] = {
    {"CreateMode", 8, 1, 0x0104, 204},
    {"ScriptName", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_00B0[] = {
    {"JointName", 24, 8, 0x0010, 0},
    {"Offset", 32, 0, 0x002C, 6},
    {"SpawnFlags", 38, 1, 0x0204, 209},
};

inline constexpr Field kFields_00B1[] = {
    {"Name", 8, 8, 0x0010, 0},
    {"RemoteName", 16, 8, 0x0010, 0},
    {"EmitterName", 24, 8, 0x0018, 0},
    {"JointName", 32, 8, 0x0018, 0},
    {"EmitterParm", 40, 8, 0x0010, 0},
    {"ArbitrationLayer", 48, 8, 0x0010, 0},
    {"Gain", 56, 4, 0x0008, 3440},
    {"Inner", 60, 4, 0x0008, 3441},
    {"Outer", 64, 4, 0x0008, 3442},
    {"RemoteFlag", 68, 1, 0x0104, 213},
    {"EmitterFlags", 69, 1, 0x0204, 214},
    {"Mode", 70, 1, 0x0104, 215},
    {"PlayOnChild", 71, 1, 0x0014, 853},
};

inline constexpr Field kFields_00B2[] = {
    {"SoundName", 8, 8, 0x0010, 0},
    {"EffectName", 16, 8, 0x0010, 0},
    {"EffectJoint", 24, 8, 0x0010, 0},
    {"TargetSpeed", 32, 4, 0x0008, 3445},
    {"Destroy", 36, 1, 0x0000, 168},
    {"Invulnerable", 37, 1, 0x0000, 169},
};

inline constexpr Field kFields_00B3[] = {
    {"TemplateSymbol", 0, 8, 0x001A, 0},
    {"AggressivePriority", 8, 1, 0x0000, 170},
    {"AggressivePriority_IsNull", 9, 1, 0x0016, 854},
};

inline constexpr Field kFields_00B4[] = {
    {"MaxRadius", 0, 4, 0x0008, 3446},
    {"Banter", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_00B5[] = {
    {"SwitchList", 0, 12, 0x0024, 55},
};

inline constexpr Field kFields_00B6[] = {
    {"Orientation", 0, 1, 0x0104, 219},
    {"FacingOrientation", 1, 1, 0x0104, 220},
    {"JointName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_00B7[] = {
    {"TemplateSymbol", 0, 8, 0x001A, 0},
    {"ActType", 8, 1, 0x0104, 221},
    {"ActType_IsNull", 9, 1, 0x0016, 855},
    {"InheritanceType", 10, 1, 0x0104, 222},
    {"InheritanceType_IsNull", 11, 1, 0x0016, 856},
    {"LODDistance", 12, 4, 0x0008, 3447},
    {"LODDistance_IsNull", 16, 1, 0x0016, 857},
};

inline constexpr Field kFields_00B8[] = {
    {"ActType_IsNull", 9, 1, 0x0016, 858},
    {"InheritanceType_IsNull", 11, 1, 0x0016, 859},
    {"LODDistance_IsNull", 16, 1, 0x0016, 860},
    {"GameObjectNames", 24, 12, 0x0024, 56},
    {"ParticleScale", 36, 4, 0x0008, 3449},
    {"AttachToJointName", 40, 8, 0x0010, 0},
    {"AttachmentName", 48, 8, 0x0010, 0},
    {"WeaponType", 56, 4, 0x0000, 172},
    {"ParticleScale_IsNull", 60, 1, 0x0016, 861},
    {"GameObjectNames_IsNull", 61, 1, 0x0016, 862},
    {"WeaponType_IsNull", 62, 1, 0x0016, 863},
    {"AttachToJointName_IsNull", 63, 1, 0x0016, 864},
    {"AttachmentName_IsNull", 64, 1, 0x0016, 865},
    {"AttachmentSide", 65, 1, 0x0104, 225},
    {"AttachmentSide_IsNull", 66, 1, 0x0016, 866},
};

inline constexpr Field kFields_00B9[] = {
    {"ActType_IsNull", 9, 1, 0x0016, 867},
    {"InheritanceType_IsNull", 11, 1, 0x0016, 868},
    {"LODDistance_IsNull", 16, 1, 0x0016, 869},
    {"GameObjectNames", 24, 12, 0x0024, 57},
    {"Duration", 36, 4, 0x0008, 3451},
    {"FadeTime", 40, 4, 0x0008, 3452},
    {"MinScale", 44, 4, 0x0008, 3453},
    {"MaxScale", 48, 4, 0x0008, 3454},
    {"MinHeight", 52, 4, 0x0008, 3455},
    {"MinWidth", 56, 4, 0x0008, 3456},
    {"Duration_IsNull", 60, 1, 0x0016, 870},
    {"FadeTime_IsNull", 61, 1, 0x0016, 871},
    {"MinScale_IsNull", 62, 1, 0x0016, 872},
    {"MaxScale_IsNull", 63, 1, 0x0016, 873},
    {"MinHeight_IsNull", 64, 1, 0x0016, 874},
    {"MinWidth_IsNull", 65, 1, 0x0016, 875},
    {"GameObjectNames_IsNull", 66, 1, 0x0016, 876},
    {"CollisionLayer", 67, 1, 0x0000, 173},
    {"CollisionLayer_IsNull", 68, 1, 0x0016, 877},
    {"AllowAttachToCharacter", 69, 1, 0x0014, 878},
    {"AllowAttachToCharacter_IsNull", 70, 1, 0x0016, 879},
};

inline constexpr Field kFields_00BA[] = {
    {"ActType_IsNull", 9, 1, 0x0016, 880},
    {"InheritanceType_IsNull", 11, 1, 0x0016, 881},
    {"LODDistance_IsNull", 16, 1, 0x0016, 882},
    {"Group", 24, 8, 0x0018, 35},
    {"Switch", 32, 8, 0x0018, 0},
    {"Group_IsNull", 40, 1, 0x0016, 883},
    {"Switch_IsNull", 41, 1, 0x0016, 884},
};

inline constexpr Field kFields_00BB[] = {
    {"ActType_IsNull", 9, 1, 0x0016, 885},
    {"InheritanceType_IsNull", 11, 1, 0x0016, 886},
    {"LODDistance_IsNull", 16, 1, 0x0016, 887},
    {"EventName", 24, 8, 0x0010, 0},
    {"EmitterName", 32, 8, 0x0010, 0},
    {"EventName_IsNull", 40, 1, 0x0016, 888},
    {"EmitterName_IsNull", 41, 1, 0x0016, 889},
    {"EmitterSource", 42, 1, 0x0104, 232},
    {"EmitterSource_IsNull", 43, 1, 0x0016, 890},
};

inline constexpr Field kFields_00BC[] = {
    {"ActType_IsNull", 9, 1, 0x0016, 891},
    {"InheritanceType_IsNull", 11, 1, 0x0016, 892},
    {"LODDistance_IsNull", 16, 1, 0x0016, 893},
    {"AttachmentName", 24, 8, 0x0010, 0},
    {"RegionMask", 32, 4, 0x0204, 235},
    {"Value", 36, 4, 0x0008, 3460},
    {"Layer", 40, 1, 0x0104, 236},
    {"Layer_IsNull", 41, 1, 0x0016, 894},
    {"Mode", 42, 1, 0x0104, 237},
    {"Mode_IsNull", 43, 1, 0x0016, 895},
    {"RegionMask_IsNull", 44, 1, 0x0016, 896},
    {"Value_IsNull", 45, 1, 0x0016, 897},
    {"Target", 46, 1, 0x0104, 238},
    {"Target_IsNull", 47, 1, 0x0016, 898},
    {"ApplyOnObject", 48, 1, 0x0014, 899},
    {"ApplyOnObject_IsNull", 49, 1, 0x0016, 900},
    {"AttachmentType", 50, 1, 0x0104, 239},
    {"AttachmentType_IsNull", 51, 1, 0x0016, 901},
    {"AttachmentName_IsNull", 52, 1, 0x0016, 902},
    {"AttachmentSide", 53, 1, 0x0104, 240},
    {"AttachmentSide_IsNull", 54, 1, 0x0016, 903},
};

inline constexpr Field kFields_00BD[] = {
    {"FXLocation", 0, 8, 0x001C, 182},
    {"Action", 8, 12, 0x0024, 58},
};

inline constexpr Field kFields_00BE[] = {
    {"Payload", 0, 0, 0x002C, 189},
    {"Switch", 24, 12, 0x0024, 60},
    {"Index", 36, 2, 0x0000, 174},
};

inline constexpr Field kFields_00BF[] = {
    {"Min", 0, 4, 0x0008, 3461},
    {"Max", 4, 4, 0x0008, 3462},
};

inline constexpr Field kFields_00C0[] = {
    {"Map", 0, 0, 0x0028, 1},
};

inline constexpr Field kFields_00C1[] = {
    {"From", 0, 1, 0x0104, 242},
    {"To", 1, 1, 0x0104, 243},
    {"InBase", 2, 1, 0x0004, 244},
    {"InRange", 3, 1, 0x0004, 245},
    {"OutBase", 4, 4, 0x0008, 3463},
    {"OutRange", 8, 4, 0x0008, 3464},
};

inline constexpr Field kFields_00C2[] = {
    {"Map", 0, 12, 0x0024, 61},
};

inline constexpr Field kFields_00C3[] = {
    {"From", 0, 1, 0x0104, 246},
    {"To", 1, 1, 0x0104, 247},
    {"Type", 2, 1, 0x0104, 248},
};

inline constexpr Field kFields_00C4[] = {
    {"Map", 0, 12, 0x0024, 62},
};

inline constexpr Field kFields_00C5[] = {
    {"Control", 0, 1, 0x0104, 249},
    {"Button", 1, 1, 0x0104, 250},
};

inline constexpr Field kFields_00C6[] = {
    {"ButtonMappings", 0, 12, 0x0024, 63},
    {"Id", 12, 1, 0x0000, 175},
    {"Name", 16, 8, 0x0018, 36},
};

inline constexpr Field kFields_00C7[] = {
    {"Layouts", 0, 12, 0x0024, 64},
};

inline constexpr Field kFields_00C8[] = {
    {"Configs", 0, 0, 0x0028, 2},
};

inline constexpr Field kFields_00C9[] = {
    {"Controls", 0, 12, 0x0024, 65},
};

inline constexpr Field kFields_00CA[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"Event", 8, 2, 0x0104, 253},
    {"EventMod", 10, 1, 0x0104, 254},
    {"Joystick", 11, 1, 0x0104, 255},
    {"ControlDown", 12, 4, 0x0104, 256},
    {"ControlUp", 16, 4, 0x0104, 257},
    {"MultiKeyDelayBetween", 20, 2, 0x0008, 3465},
    {"HoldDownDelayedTime", 22, 2, 0x0008, 3466},
    {"PreventInteractionExpression", 24, 8, 0x0030, 65535},
    {"LamsId", 32, 4, 0x0000, 176},
};

inline constexpr Field kFields_00CB[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"Options", 8, 12, 0x0024, 66},
};

inline constexpr Field kFields_00CC[] = {
    {"Settings", 0, 12, 0x0024, 67},
    {"LastInput", 16, 8, 0x001C, 201},
};

inline constexpr Field kFields_00CD[] = {
    {"MaxGestureTime", 0, 2, 0x0008, 3467},
    {"MinGestureTime", 2, 2, 0x0008, 3468},
    {"SwipeDistanceThreshold", 4, 2, 0x0008, 3469},
};

inline constexpr Field kFields_00CE[] = {
    {"Control", 0, 8, 0x001C, 192},
    {"Analog", 8, 8, 0x001C, 194},
    {"Digital", 16, 8, 0x001C, 196},
};

inline constexpr Field kFields_00CF[] = {
    {"Controller", 0, 0, 0x0028, 3},
};

inline constexpr Field kFields_00D1[] = {
    {"RefJoint", 24, 8, 0x0010, 0},
    {"MinAngle", 32, 4, 0x0008, 3474},
    {"MaxAngle", 36, 4, 0x0008, 3475},
    {"DriverName", 40, 8, 0x0010, 38},
};

inline constexpr Field kFields_00D2[] = {
    {"RefJoint", 24, 8, 0x0010, 0},
    {"MinAngle", 32, 4, 0x0008, 3478},
    {"MaxAngle", 36, 4, 0x0008, 3479},
    {"DriverName", 40, 8, 0x0010, 40},
};

inline constexpr Field kFields_00D3[] = {
    {"RefJoint", 24, 8, 0x0010, 0},
    {"HMinAngle", 32, 4, 0x0008, 3482},
    {"HMaxAngle", 36, 4, 0x0008, 3483},
    {"VMinAngle", 40, 4, 0x0008, 3484},
    {"VMaxAngle", 44, 4, 0x0008, 3485},
    {"DriverNameLeftRight", 48, 8, 0x0010, 42},
    {"DriverNameUpDown", 56, 8, 0x0010, 43},
};

inline constexpr Field kFields_00D4[] = {
    {"RefJoint", 24, 8, 0x0010, 0},
    {"LeftDistance", 32, 4, 0x0008, 3488},
    {"RightDistance", 36, 4, 0x0008, 3489},
    {"UpDistance", 40, 4, 0x0008, 3490},
    {"DownDistance", 44, 4, 0x0008, 3491},
    {"NearDistance", 48, 4, 0x0008, 3492},
    {"FarDistance", 52, 4, 0x0008, 3493},
    {"DriverNameLeftRight", 56, 8, 0x0010, 45},
    {"DriverNameUpDown", 64, 8, 0x0010, 46},
    {"DriverNameNearFar", 72, 8, 0x0010, 47},
    {"FreezeBlend", 80, 1, 0x0014, 904},
};

inline constexpr Field kFields_00D5[] = {
    {"HMinAngle", 24, 4, 0x0008, 3496},
    {"HMaxAngle", 28, 4, 0x0008, 3497},
    {"VMinAngle", 32, 4, 0x0008, 3498},
    {"VMaxAngle", 36, 4, 0x0008, 3499},
    {"CameraYDriverOut", 40, 8, 0x0010, 49},
    {"CameraXDriverOut", 48, 8, 0x0010, 50},
};

inline constexpr Field kFields_00D6[] = {
    {"Stick", 24, 1, 0x0104, 287},
    {"AnimRefHMinAngle", 28, 4, 0x0008, 3502},
    {"AnimRefHMaxAngle", 32, 4, 0x0008, 3503},
    {"AnimRefVMinAngle", 36, 4, 0x0008, 3504},
    {"AnimRefVMaxAngle", 40, 4, 0x0008, 3505},
    {"HMinAngle", 44, 4, 0x0008, 3506},
    {"HMaxAngle", 48, 4, 0x0008, 3507},
    {"VMinAngle", 52, 4, 0x0008, 3508},
    {"VMaxAngle", 56, 4, 0x0008, 3509},
    {"Damping", 60, 4, 0x0008, 3510},
    {"MinimumLinearRate", 64, 4, 0x0008, 3511},
    {"StickMagDriverOut", 72, 8, 0x0010, 52},
    {"StickDirDriverOut", 80, 8, 0x0010, 53},
    {"StickXDriverOut", 88, 8, 0x0010, 54},
    {"StickYDriverOut", 96, 8, 0x0010, 55},
};

inline constexpr Field kFields_00D7[] = {
    {"Stick", 24, 1, 0x0104, 292},
    {"Damping", 28, 4, 0x0008, 3514},
    {"MinimumLinearRate", 32, 4, 0x0008, 3515},
};

inline constexpr Field kFields_00D8[] = {
    {"FadeInTime", 0, 4, 0x0008, 3516},
    {"FadeInRandom", 4, 4, 0x0008, 3517},
    {"FadeOutTime", 8, 4, 0x0008, 3518},
    {"FadeOutRandom", 12, 4, 0x0008, 3519},
    {"EventType", 16, 1, 0x0104, 293},
};

inline constexpr Field kFields_00D9[] = {
    {"WeatherEvents", 0, 12, 0x0024, 68},
    {"InitialDelay", 12, 4, 0x0008, 3520},
    {"IntervalTimeMin", 16, 4, 0x0008, 3521},
    {"IntervalTimeMax", 20, 4, 0x0008, 3522},
    {"MinDistanceFromTarget", 24, 4, 0x0008, 3523},
    {"MaxDistanceFromTarget", 28, 4, 0x0008, 3524},
    {"CountMin", 32, 2, 0x0000, 185},
    {"CountMax", 34, 2, 0x0000, 186},
    {"RepeatCount", 36, 2, 0x0000, 187},
    {"SpawnLocation", 38, 1, 0x0104, 294},
};

inline constexpr Field kFields_00DA[] = {
    {"WeatherSequences", 0, 12, 0x0024, 69},
    {"WeatherCategory", 12, 1, 0x0104, 295},
    {"WeatherName", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_00DB[] = {
    {"WeatherTypes", 0, 12, 0x0024, 70},
};

inline constexpr Field kFields_00DC[] = {
    {"GameObjectName", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_00DD[] = {
    {"Duration", 20, 4, 0x0008, 3533},
    {"GameObjectNames", 24, 12, 0x0024, 71},
    {"MinScale", 36, 4, 0x0008, 3534},
    {"MaxScale", 40, 4, 0x0008, 3535},
    {"CollisionLayer", 44, 1, 0x0000, 188},
};

inline constexpr Field kFields_00DE[] = {
    {"WetnessAmount", 20, 4, 0x0008, 3540},
    {"WetnessHeight", 24, 4, 0x0008, 3541},
    {"WetnessHeightTransition", 28, 4, 0x0008, 3542},
};

inline constexpr Field kFields_00DF[] = {
    {"Duration", 20, 4, 0x0008, 3547},
    {"ParmName", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_00E0[] = {
    {"SoundEvent", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_00E1[] = {
    {"Layer", 20, 1, 0x0104, 302},
    {"RegionMask", 24, 4, 0x0204, 303},
    {"Value", 28, 4, 0x0008, 3556},
};

inline constexpr Field kFields_00E2[] = {
    {"AttributeName", 0, 8, 0x0010, 0},
    {"Index", 8, 2, 0x0000, 189},
};

inline constexpr Field kFields_00E3[] = {
    {"Attribute", 0, 8, 0x0010, 0},
    {"Value", 8, 4, 0x0008, 3557},
};

inline constexpr Field kFields_00E4[] = {
    {"Attribute", 0, 8, 0x0010, 0},
    {"Scale", 8, 4, 0x0008, 3558},
    {"RefCreature", 12, 1, 0x0104, 304},
    {"FloorValue", 13, 1, 0x0014, 905},
    {"RefCreatureDynamicFlagFilter", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_00E5[] = {
    {"Attributes", 0, 12, 0x0024, 72},
    {"Base", 12, 4, 0x0008, 3559},
    {"Min", 16, 4, 0x0008, 3560},
    {"Max", 20, 4, 0x0008, 3561},
    {"ScaleResult", 24, 4, 0x0008, 3562},
    {"EaseResultMax", 28, 4, 0x0008, 3563},
    {"LogEase", 32, 4, 0x0008, 3564},
    {"Type", 36, 1, 0x0104, 305},
    {"EaseType", 37, 1, 0x0104, 306},
    {"ModFloatFlags", 38, 1, 0x0204, 307},
};

inline constexpr Field kFields_00E6[] = {
    {"DataPoint", 0, 12, 0x0024, 73},
};

inline constexpr Field kFields_00E7[] = {
    {"SelfDynamicFlagFilterList", 0, 12, 0x0024, 74},
    {"Type", 12, 1, 0x0204, 308},
    {"StatusMeterDamageFilter", 13, 1, 0x0104, 309},
    {"ExcludeDynamicFlags", 14, 1, 0x0014, 906},
    {"AttributeStatus", 16, 8, 0x001C, 1280},
    {"UserCurve", 24, 8, 0x001C, 230},
    {"ModFloat", 32, 8, 0x001C, 229},
    {"Name", 40, 8, 0x0010, 0},
    {"HitFlags", 48, 8, 0x0204, 310},
    {"ExcludeHitFlags", 56, 8, 0x0204, 311},
    {"StatusMeterName", 64, 8, 0x0010, 0},
    {"StatusMeterDamageWhenExpression", 72, 8, 0x0030, 65535},
};

inline constexpr Field kFields_00E8[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Value", 8, 4, 0x0000, 190},
    {"Display", 12, 1, 0x0014, 907},
};

inline constexpr Field kFields_00E9[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Value", 8, 1, 0x0000, 191},
    {"Display", 9, 1, 0x0014, 908},
};

inline constexpr Field kFields_00EA[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Value", 8, 4, 0x0008, 3565},
    {"Display", 12, 1, 0x0014, 909},
};

inline constexpr Field kFields_00EB[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Value", 8, 8, 0x0018, 0},
    {"Display", 16, 1, 0x0014, 910},
};

inline constexpr Field kFields_00ED[] = {
    {"TrailEdgeShape", 0, 0, 0x002C, 236},
    {"AttachName", 16, 8, 0x0010, 0},
    {"TrailType", 24, 8, 0x0010, 57},
    {"WeaponType", 32, 4, 0x0000, 192},
    {"TrailDuration", 36, 4, 0x0008, 3574},
    {"TrailScaleWidth", 40, 4, 0x0008, 3575},
    {"TrailEdgeMinWidth", 44, 4, 0x0008, 3576},
    {"FadeOutTime", 48, 4, 0x0008, 3577},
    {"TrailIndex", 52, 1, 0x0000, 193},
    {"TrailMode", 53, 1, 0x0104, 312},
};

inline constexpr Field kFields_00EE[] = {
    {"IgnorePartFlags", 0, 8, 0x0204, 313},
    {"EnemyID", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_00EF[] = {
    {"IgnorePrecisionTargets", 0, 12, 0x0024, 75},
    {"MinDistance", 12, 4, 0x0008, 3578},
    {"EnemyDynamicFlagWeights", 16, 12, 0x0024, 76},
    {"MaxDistance", 28, 4, 0x0008, 3579},
    {"NeutralMinAngle", 32, 4, 0x0008, 3580},
    {"NeutralMaxAngle", 36, 4, 0x0008, 3581},
    {"DeflectedMinAngle", 40, 4, 0x0008, 3582},
    {"DeflectedMaxAngle", 44, 4, 0x0008, 3583},
    {"CamRelativeMinAngle", 48, 4, 0x0008, 3584},
    {"CamRelativeMaxAngle", 52, 4, 0x0008, 3585},
    {"MinHeight", 56, 4, 0x0008, 3586},
    {"MaxHeight", 60, 4, 0x0008, 3587},
    {"HeightOffset", 64, 4, 0x0008, 3588},
    {"ScreenCenterInner", 68, 4, 0x0008, 3589},
    {"ScreenCenterOuter", 72, 4, 0x0008, 3590},
    {"MinTimeSinceVisible", 76, 4, 0x0008, 3591},
    {"MaxTimeSinceVisible", 80, 4, 0x0008, 3592},
    {"VisibilityRefreshTime", 84, 4, 0x0008, 3593},
    {"DistanceWeight", 88, 4, 0x0008, 3594},
    {"AngleWeight", 92, 4, 0x0008, 3595},
    {"HeightWeight", 96, 4, 0x0008, 3596},
    {"CompatibilityWeight", 100, 4, 0x0008, 3597},
    {"CurrentTargetWeight", 104, 4, 0x0008, 3598},
    {"ScreenCenterWeight", 108, 4, 0x0008, 3599},
    {"VisibilityWeight", 112, 4, 0x0008, 3600},
    {"OnScreenWeight", 116, 4, 0x0008, 3601},
    {"CamRelativeWeight", 120, 4, 0x0008, 3602},
    {"FailFlags", 124, 2, 0x0204, 314},
};

inline constexpr Field kFields_00F0[] = {
    {"MinHeight", 0, 4, 0x0008, 3603},
    {"MaxHeight", 4, 4, 0x0008, 3604},
    {"RightAngleDeg", 8, 4, 0x0008, 3605},
    {"LeftAngleDeg", 12, 4, 0x0008, 3606},
    {"RightClippingPlaneOffset", 16, 4, 0x0008, 3607},
    {"LeftClippingPlaneOffset", 20, 4, 0x0008, 3608},
};

inline constexpr Field kFields_00F1[] = {
    {"IsStun", 0, 1, 0x0014, 911},
    {"UseInteractPromptAsPositionIfStunned", 1, 1, 0x0014, 912},
};

inline constexpr Field kFields_00F2[] = {
    {"PlayerDistanceIgnoreAOOThreshold", 0, 4, 0x0008, 3609},
    {"PlayerDistanceAddBackToAOOThreshold", 4, 4, 0x0008, 3610},
};

inline constexpr Field kFields_00F3[] = {
    {"EncounterEndedArbitrationDelay", 0, 4, 0x0008, 3611},
    {"InterruptBleedLength", 4, 4, 0x0008, 3612},
};

inline constexpr Field kFields_00F4[] = {
    {"Length", 0, 4, 0x0008, 3613},
    {"Height", 4, 4, 0x0008, 3614},
    {"MaxAngleForStraightHit", 8, 4, 0x0008, 3615},
    {"Direction", 12, 1, 0x0104, 315},
};

inline constexpr Field kFields_00F5[] = {
    {"DistanceLessThan", 0, 2, 0x0008, 3616},
    {"HitDirection", 2, 1, 0x0104, 316},
    {"WallMaterial", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_00F6[] = {
    {"Joints", 0, 12, 0x0024, 77},
};

inline constexpr Field kFields_00F7[] = {
    {"PickupFilterList", 0, 12, 0x0024, 78},
    {"FilterRadius", 12, 4, 0x0008, 3617},
    {"EnemyIDFilterList", 16, 12, 0x0024, 79},
    {"MeterValue", 28, 4, 0x0008, 3618},
    {"MarkerIDFilterList", 32, 12, 0x0024, 80},
    {"Team", 44, 1, 0x0104, 317},
    {"MeterCompareType", 45, 1, 0x0104, 318},
    {"EnemyContextFilterList", 48, 12, 0x0024, 81},
    {"DynamicFlagFilterList", 64, 12, 0x0024, 82},
    {"MeterToCheck", 80, 8, 0x0010, 0},
    {"HitFlags", 88, 8, 0x0204, 319},
};

inline constexpr Field kFields_00F8[] = {
    {"Power", 0, 4, 0x0008, 3619},
    {"Tint", 16, 0, 0x002C, 1},
};

inline constexpr Field kFields_00F9[] = {
    {"ConfigName", 0, 8, 0x0010, 0},
    {"Probability", 8, 4, 0x0008, 3624},
};

inline constexpr Field kFields_00FA[] = {
    {"Type", 0, 1, 0x0104, 320},
    {"Flags", 1, 1, 0x0204, 321},
};

inline constexpr Field kFields_00FB[] = {
    {"Offset", 2, 0, 0x002C, 6},
};

inline constexpr Field kFields_00FC[] = {
    {"Name", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_00FD[] = {
    {"Breakpoint", 2, 0, 0x002C, 6},
    {"This", 8, 8, 0x001C, 250},
    {"That", 16, 8, 0x001C, 250},
};

inline constexpr Field kFields_00FE[] = {
    {"CenterRatio", 24, 4, 0x0008, 3637},
};

inline constexpr Field kFields_00FF[] = {
    {"Flags", 0, 1, 0x0204, 330},
    {"YRange", 4, 4, 0x0008, 3638},
    {"XZRange", 8, 4, 0x0008, 3639},
    {"Angle", 12, 4, 0x0008, 3640},
    {"GrabbersMaxAngle", 16, 4, 0x0008, 3641},
    {"BehindRange", 20, 4, 0x0008, 3642},
    {"BehindWidth", 24, 4, 0x0008, 3643},
    {"GrabbersSlot", 32, 8, 0x001C, 1186},
};

inline constexpr Field kFields_0100[] = {
    {"Label", 0, 8, 0x0010, 0},
    {"StringData", 8, 8, 0x0010, 0},
    {"NumberData", 16, 4, 0x0008, 3644},
    {"Comparison", 20, 1, 0x0104, 331},
};

inline constexpr Field kFields_0101[] = {
    {"TargetJointId", 0, 1, 0x0000, 194},
    {"Concussion", 8, 8, 0x001C, 268},
    {"StickyBomb", 16, 8, 0x001C, 258},
    {"CameraShake", 24, 8, 0x001C, 997},
    {"ForceFeedback", 32, 8, 0x001C, 274},
    {"Haptic", 40, 8, 0x001C, 481},
    {"Pickup", 48, 8, 0x001C, 1183},
    {"EventName", 56, 8, 0x0010, 0},
};

inline constexpr Field kFields_0102[] = {
    {"MinFuse", 0, 4, 0x0008, 3645},
    {"MaxFuse", 4, 4, 0x0008, 3646},
    {"EffectName", 8, 8, 0x0018, 58},
    {"Concussion", 16, 8, 0x001C, 268},
    {"Heap", 24, 4, 0x0000, 195},
};

inline constexpr Field kFields_0103[] = {
    {"PowerUpPoints", 0, 2, 0x0000, 196},
    {"MessageIdx", 2, 2, 0x0000, 197},
    {"IncStat", 4, 1, 0x0104, 332},
};

inline constexpr Field kFields_0104[] = {
    {"ID", 0, 1, 0x0000, 198},
    {"ValidThrowableResponse", 8, 8, 0x0010, 59},
};

inline constexpr Field kFields_0105[] = {
    {"Contexts", 0, 12, 0x0024, 83},
    {"DamageScale", 12, 4, 0x0008, 3647},
    {"CollisionSphereList", 16, 12, 0x0024, 84},
    {"AllowHit", 28, 1, 0x0014, 913},
    {"ExcludeHitFlags", 29, 1, 0x0014, 914},
    {"ExcludeContexts", 30, 1, 0x0014, 915},
    {"HitFlags", 32, 8, 0x0204, 333},
};

inline constexpr Field kFields_0106[] = {
    {"OrbEmitter", 0, 8, 0x001C, 321},
    {"AirOrbEmitter", 8, 8, 0x001C, 321},
    {"Bonus", 16, 8, 0x001C, 259},
    {"AirBonus", 24, 8, 0x001C, 259},
    {"PristineStage", 32, 0, 0x002C, 1450},
    {"DefaultBrokenStage", 48, 0, 0x002C, 1450},
    {"BrokenStages", 64, 12, 0x0024, 87},
    {"OpaqueTime", 76, 2, 0x0008, 3650},
    {"FadeTime", 78, 2, 0x0008, 3651},
    {"FadeAndDisappearAfterBroken", 80, 1, 0x0014, 916},
    {"SwitchFromAnimToPhysicsTime", 82, 2, 0x0008, 3652},
    {"TargetOffset", 84, 0, 0x002C, 6},
    {"Flags", 90, 1, 0x0204, 334},
    {"ShouldRecycleAfterBreak", 91, 1, 0x0014, 917},
    {"UseExtendedCullingRadius", 92, 1, 0x0014, 918},
    {"UseRollingFrictionOnSnow", 93, 1, 0x0014, 919},
    {"RollingFriction", 94, 2, 0x0008, 3656},
    {"SuckToTargetRadius", 96, 2, 0x0008, 3657},
};

inline constexpr Field kFields_0107[] = {
    {"DecalNames", 0, 32, 0x0024, 88},
    {"MFXSwitches", 32, 0, 0x002C, 181},
    {"WeaponType", 44, 4, 0x0000, 200},
    {"AttachmentName", 48, 8, 0x0010, 0},
    {"JointName", 56, 8, 0x0010, 0},
    {"PositionOverrideJointName", 64, 8, 0x0010, 0},
    {"FXName", 72, 8, 0x0010, 0},
    {"CollisionRayStartOffset", 80, 4, 0x0008, 3658},
    {"CollisionRayLength", 84, 4, 0x0008, 3659},
    {"OffsetX", 88, 4, 0x0008, 3660},
    {"OffsetY", 92, 4, 0x0008, 3661},
    {"OffsetZ", 96, 4, 0x0008, 3662},
    {"DecalSizeMin", 100, 4, 0x0008, 3663},
    {"DecalSizeMax", 104, 4, 0x0008, 3664},
    {"DecalDuration", 108, 4, 0x0008, 3665},
    {"CollisionRayDirection", 112, 1, 0x0104, 335},
    {"FXOrientation", 113, 1, 0x0104, 336},
    {"CollideWithEnvironment", 114, 1, 0x0014, 920},
    {"CollideWithCharacters", 115, 1, 0x0014, 921},
};

inline constexpr Field kFields_0108[] = {
    {"MFXTrigger", 0, 0, 0x002C, 263},
    {"TriggerTime", 120, 4, 0x0008, 3674},
};

inline constexpr Field kFields_0109[] = {
    {"EffectName", 0, 8, 0x0010, 0},
    {"AttachmentName", 8, 8, 0x0010, 0},
    {"EffectTint", 16, 0, 0x002C, 0},
    {"WeaponType", 24, 4, 0x0000, 203},
    {"EffectScale", 28, 4, 0x0008, 3679},
    {"AttachmentEffectEmissionModulate", 32, 4, 0x0008, 3680},
    {"PercentFadeStart", 36, 4, 0x0008, 3681},
    {"PercentFadeEnd", 40, 4, 0x0008, 3682},
    {"AttachmentSide", 44, 1, 0x0104, 339},
    {"Flags", 45, 1, 0x0204, 340},
};

inline constexpr Field kFields_010A[] = {
    {"Effect", 0, 8, 0x001C, 46},
    {"EffectName", 8, 8, 0x0018, 0},
    {"TweenIn", 16, 0, 0x002C, 12},
    {"TweenOut", 40, 0, 0x002C, 12},
    {"On", 64, 4, 0x0008, 3695},
    {"Off", 68, 4, 0x0008, 3696},
};

inline constexpr Field kFields_010B[] = {
    {"ShrapnelConcussionLevels", 0, 12, 0x0024, 92},
    {"EmpoweredScaleWidth", 12, 4, 0x0008, 3697},
    {"EmpoweredScaleLength", 16, 4, 0x0008, 3698},
    {"BlastSpeed", 20, 4, 0x0008, 3699},
    {"BlastUpSpeed", 24, 4, 0x0008, 3700},
    {"PlayFX", 32, 8, 0x001C, 272},
};

inline constexpr Field kFields_010C[] = {
    {"DurationList", 0, 12, 0x0024, 93},
    {"WeaponType", 12, 4, 0x0000, 204},
    {"TimedMFXTriggerList", 16, 12, 0x0024, 94},
    {"Heap", 28, 4, 0x0000, 205},
    {"EffectList", 32, 12, 0x0024, 95},
    {"OffsetX", 44, 4, 0x0008, 3702},
    {"AttachName", 48, 8, 0x0010, 0},
    {"PlayFX", 56, 8, 0x001C, 272},
    {"JointName", 64, 8, 0x0010, 0},
    {"SoundOnHitName", 72, 8, 0x0010, 0},
    {"Collision", 80, 8, 0x001C, 396},
    {"Context", 88, 8, 0x0010, 0},
    {"HitFlags", 96, 8, 0x0204, 341},
    {"LuaHookOverride", 104, 8, 0x0018, 0},
    {"SyncedSound", 112, 8, 0x0018, 0},
    {"SyncedSoundEmitter", 120, 8, 0x0018, 0},
    {"Payload", 128, 8, 0x001C, 257},
    {"ShrapnelConcussion", 136, 8, 0x001C, 267},
    {"OffsetY", 144, 4, 0x0008, 3703},
    {"OffsetZ", 148, 4, 0x0008, 3704},
    {"RigidBodyImpulseUpward", 152, 4, 0x0008, 3705},
    {"RigidBodyImpulseForward", 156, 4, 0x0008, 3706},
    {"Flags", 160, 2, 0x0204, 342},
    {"LoopCnt", 162, 2, 0x0000, 206},
    {"DataType", 164, 1, 0x0105, 343},
    {"SendLuaHook", 165, 1, 0x0014, 924},
};

inline constexpr Field kFields_010D[] = {
    {"Type", 168, 1, 0x0104, 347},
    {"PieAngle", 172, 4, 0x0008, 3713},
    {"RadiusList", 176, 12, 0x0024, 99},
    {"DonutHeight", 188, 4, 0x0008, 3715},
};

inline constexpr Field kFields_010E[] = {
    {"PlaneList", 168, 12, 0x0024, 103},
};

inline constexpr Field kFields_010F[] = {
    {"BlockList", 168, 12, 0x0024, 107},
};

inline constexpr Field kFields_0110[] = {
    {"SoundAction", 0, 12, 0x0024, 108},
    {"WeaponType", 12, 4, 0x0000, 216},
    {"SoundWindowAction", 16, 12, 0x0024, 109},
    {"NumParticleScale", 28, 4, 0x0008, 3728},
    {"AttachName", 32, 8, 0x0010, 0},
    {"GOName", 40, 8, 0x0010, 0},
    {"JointName", 48, 8, 0x0010, 0},
    {"FXTint", 56, 0, 0x002C, 0},
    {"pointer_0", 64, 8, 0x001C, 177},
    {"pointer_1", 72, 8, 0x001C, 972},
    {"ScaleX", 80, 4, 0x0008, 3733},
    {"ScaleY", 84, 4, 0x0008, 3734},
    {"ScaleZ", 88, 4, 0x0008, 3735},
    {"OffsetX", 92, 4, 0x0008, 3736},
    {"OffsetY", 96, 4, 0x0008, 3737},
    {"OffsetZ", 100, 4, 0x0008, 3738},
    {"RotateX", 104, 4, 0x0008, 3739},
    {"RotateY", 108, 4, 0x0008, 3740},
    {"RotateZ", 112, 4, 0x0008, 3741},
    {"Flags", 116, 4, 0x0204, 354},
    {"Heap", 120, 4, 0x0000, 217},
    {"OrderOfOperations", 124, 1, 0x0104, 355},
    {"AccessibilityHighlightCategory", 125, 1, 0x0104, 356},
};

inline constexpr Field kFields_0111[] = {
    {"List", 0, 12, 0x0024, 110},
};

inline constexpr Field kFields_0112[] = {
    {"Joint", 80, 8, 0x0010, 0},
    {"Inner", 88, 4, 0x0008, 3756},
    {"Outer", 92, 4, 0x0008, 3757},
    {"SMFrequency", 96, 4, 0x0008, 3758},
    {"SMAmplitude", 100, 4, 0x0008, 3759},
    {"SMPhase", 104, 4, 0x0008, 3760},
    {"SMBias", 108, 4, 0x0008, 3761},
    {"LMFrequency", 112, 4, 0x0008, 3762},
    {"LMAmplitude", 116, 4, 0x0008, 3763},
    {"LMPhase", 120, 4, 0x0008, 3764},
    {"LMBias", 124, 4, 0x0008, 3765},
    {"Attack", 128, 4, 0x0008, 3766},
    {"Decay", 132, 4, 0x0008, 3767},
    {"Sustain", 136, 4, 0x0008, 3768},
    {"Release", 140, 4, 0x0008, 3769},
    {"SMWaveform", 144, 1, 0x0104, 359},
    {"LMWaveform", 145, 1, 0x0104, 360},
    {"EnableOnPS5", 146, 1, 0x0014, 929},
};

inline constexpr Field kFields_0113[] = {
    {"Project", 0, 1, 0x0104, 361},
};

inline constexpr Field kFields_0114[] = {
    {"MaxRigidBodies", 0, 4, 0x0000, 220},
    {"MaxShapes", 4, 4, 0x0000, 221},
    {"MaxJoints", 8, 4, 0x0000, 222},
    {"MaxContacts", 12, 4, 0x0000, 223},
    {"MaxNonContactPairs", 16, 4, 0x0000, 224},
    {"MaxCCDKilobytes", 20, 4, 0x0000, 225},
};

inline constexpr Field kFields_0115[] = {
    {"CollisionSet", 0, 8, 0x001C, 276},
    {"SoundSet", 8, 8, 0x001C, 276},
    {"CameraSet", 16, 8, 0x001C, 276},
    {"EntitySet", 24, 8, 0x001C, 276},
    {"EffectSet", 32, 8, 0x001C, 276},
    {"VisibilitySet", 40, 8, 0x001C, 276},
    {"DisabledSet", 48, 8, 0x001C, 276},
};

inline constexpr Field kFields_0116[] = {
    {"CreatureCategory", 0, 1, 0x0104, 362},
    {"CameraDistanceScaleMultiplier", 4, 4, 0x0008, 3770},
    {"CameraDistanceOffset", 8, 4, 0x0008, 3771},
    {"LODOffset", 12, 4, 0x0000, 226},
};

inline constexpr Field kFields_0117[] = {
    {"VolumetricFogMaxDimensions", 0, 0, 0x002C, 8},
    {"MaxRenderFraction", 12, 4, 0x0008, 3772},
    {"CreatureLODInfo", 16, 12, 0x0024, 111},
    {"MedRenderFraction", 28, 4, 0x0008, 3773},
    {"GPUBudgetName", 32, 8, 0x0010, 60},
    {"MinRenderFraction", 40, 4, 0x0008, 3774},
    {"MinRenderFractionVrr", 44, 4, 0x0008, 3775},
    {"TessellationLengthFactor", 48, 4, 0x0008, 3776},
    {"TessellationFadeStartDistance", 52, 4, 0x0008, 3777},
    {"TessellationFadeEndDistance", 56, 4, 0x0008, 3778},
    {"GTAORadius", 60, 4, 0x0008, 3779},
    {"SSRMinimumGloss", 64, 4, 0x0008, 3780},
    {"SSRNormalFadeStart", 68, 4, 0x0008, 3781},
    {"SSRNormalFadeEnd", 72, 4, 0x0008, 3782},
    {"BrdfCullThreshold", 76, 4, 0x0008, 3783},
    {"HeightFieldDetailModelCullingDistance", 80, 4, 0x0008, 3784},
    {"ShadowTextureWidth", 84, 4, 0x0008, 3785},
    {"ShadowTextureHeight", 88, 4, 0x0008, 3786},
    {"ShadowCascadeDistScale0", 92, 4, 0x0008, 3787},
    {"ShadowCascadeDistScale1", 96, 4, 0x0008, 3788},
    {"LocalLightShadowResolutionScale", 100, 4, 0x0008, 3789},
    {"LodDistanceScale", 104, 4, 0x0008, 3790},
    {"VolumetricFogMaxResolutionDistance", 108, 4, 0x0008, 3791},
    {"WindTurbulenceMaxDistance", 112, 4, 0x0008, 3792},
    {"WindTurbulenceTransitionDistance", 116, 4, 0x0008, 3793},
    {"WindTurbulenceCullingMinWidthInPixels", 120, 4, 0x0008, 3794},
    {"LightShadowCullingDistanceScale", 124, 4, 0x0008, 3795},
    {"BreakableLifetimeMultiplier", 128, 4, 0x0008, 3796},
    {"TextureHeapMB", 132, 4, 0x0008, 3797},
    {"SSRRaySteps", 136, 2, 0x0000, 230},
    {"SSRWaterRaySteps", 138, 2, 0x0000, 231},
    {"ShadowCascadeResolution", 140, 2, 0x0000, 232},
    {"LocalLightShadowResolutionMaximum", 142, 2, 0x0000, 233},
    {"Resolution", 144, 1, 0x0104, 363},
    {"TargetFrameRate", 145, 1, 0x0104, 364},
    {"IncreaseResolutionToMatchCPUFramerateEnabled", 146, 1, 0x0014, 930},
    {"TessellationEnabled", 147, 1, 0x0014, 931},
    {"ShadowTessellationMode", 148, 1, 0x0104, 365},
    {"EnableSSGI", 149, 1, 0x0014, 932},
    {"EnableSSDO", 150, 1, 0x0014, 933},
    {"GTAORotationSteps", 151, 1, 0x0000, 234},
    {"GTAOOffsetSteps", 152, 1, 0x0000, 235},
    {"SSREnabled", 153, 1, 0x0014, 934},
    {"RaytracedCubemapsEnabled", 154, 1, 0x0014, 935},
    {"MinimodelShadowsEnabled", 155, 1, 0x0014, 936},
    {"HeightFieldShadowsEnabled", 156, 1, 0x0014, 937},
    {"HeightFieldDetailModelShadowsEnabled", 157, 1, 0x0014, 938},
    {"IncludeDepthPrepassInShadows", 158, 1, 0x0014, 939},
    {"ContactHardeningShadowsEnabled", 159, 1, 0x0014, 940},
    {"CheapContactHardeningShadowsEnabled", 160, 1, 0x0014, 941},
    {"AnisotropyRatio", 161, 1, 0x0104, 366},
    {"AnisotropyThreshold", 162, 1, 0x0000, 236},
    {"AnisotropyHighThreshold", 163, 1, 0x0000, 237},
    {"SH3SkyLightingEnabled", 164, 1, 0x0014, 942},
    {"HighQualityLightsEnabled", 165, 1, 0x0014, 943},
    {"TextureUpscalingEnabled", 166, 1, 0x0014, 944},
};

inline constexpr Field kFields_0118[] = {
    {"PlatformPresets", 0, 0, 0x0028, 4},
};

inline constexpr Field kFields_0119[] = {
    {"NameToCategoryMap", 0, 0, 0x0028, 5},
};

inline constexpr Field kFields_011A[] = {
    {"Preset", 0, 1, 0x0104, 369},
};

inline constexpr Field kFields_011B[] = {
    {"WadNameList", 0, 12, 0x0024, 112},
    {"HeapSizeList", 16, 12, 0x0024, 113},
    {"VRAMSizeList", 32, 12, 0x0024, 114},
    {"GroupList", 48, 12, 0x0024, 115},
};

inline constexpr Field kFields_011C[] = {
    {"LoadByPlayerLoc", 0, 12, 0x0024, 116},
    {"LoadByLogicGroup", 16, 12, 0x0024, 117},
    {"LogicGroupConditions", 32, 12, 0x0024, 118},
    {"WadLoadGroupInfo", 48, 0, 0x002C, 1458},
};

inline constexpr Field kFields_011D[] = {
    {"EffectHeapBlockSize", 0, 4, 0x0000, 242},
    {"EffectHeapTotalSize", 4, 4, 0x0000, 243},
    {"EffectHeapSafeAllocationThreshold", 8, 4, 0x0008, 3798},
};

inline constexpr Field kFields_011E[] = {
    {"DamageMult", 0, 4, 0x0008, 3799},
    {"AIDamageMult", 4, 4, 0x0008, 3800},
    {"MaxHealthMult", 8, 4, 0x0008, 3801},
    {"StunMult", 12, 4, 0x0008, 3802},
    {"HealthOrbMult", 16, 4, 0x0008, 3803},
    {"WeaponOrbMult", 20, 4, 0x0008, 3804},
};

inline constexpr Field kFields_011F[] = {
    {"RampType", 0, 1, 0x0104, 370},
    {"Start", 4, 4, 0x0008, 3805},
    {"End", 8, 4, 0x0008, 3806},
    {"Duration", 12, 4, 0x0008, 3807},
};

inline constexpr Field kFields_0120[] = {
    {"RegenMode", 0, 1, 0x0204, 371},
    {"PauseDelay", 8, 0, 0x002C, 229},
    {"PauseDelayFull", 48, 0, 0x002C, 229},
    {"PauseDelayEmpty", 88, 0, 0x002C, 229},
    {"RegenTickAmount", 128, 0, 0x002C, 229},
    {"RegenScaleTickAmount", 168, 0, 0x002C, 287},
    {"RegenTickInterval", 184, 0, 0x002C, 287},
    {"RegenMinThreshold", 200, 0, 0x002C, 229},
    {"RegenMaxThreshold", 240, 0, 0x002C, 229},
    {"RegenTickAmountImmune", 280, 0, 0x002C, 229},
    {"RegenMin", 320, 0, 0x002C, 229},
    {"RegenMax", 360, 0, 0x002C, 229},
    {"RegenRate", 400, 0, 0x002C, 229},
    {"MaxRegen", 440, 0, 0x002C, 229},
};

inline constexpr Field kFields_0121[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Max", 8, 0, 0x002C, 229},
    {"Min", 48, 0, 0x002C, 229},
};

inline constexpr Field kFields_0122[] = {
    {"BarColor", 0, 0, 0x002C, 1},
    {"BackgroundColor", 16, 0, 0x002C, 1},
    {"JointName", 32, 8, 0x0010, 61},
    {"Height", 40, 4, 0x0008, 3900},
    {"Width", 44, 4, 0x0008, 3901},
    {"ScreenPositionX", 48, 4, 0x0008, 3902},
    {"ScreenPositionY", 52, 4, 0x0008, 3903},
    {"VerticalOffset", 56, 4, 0x0008, 3904},
    {"MinThreshold", 60, 4, 0x0008, 3905},
    {"MaxThreshold", 64, 4, 0x0008, 3906},
    {"DisplayDuration", 68, 4, 0x0008, 3907},
    {"FadeTime", 72, 4, 0x0008, 3908},
    {"DisplayMode", 76, 1, 0x0104, 413},
    {"IsHealthBar", 77, 1, 0x0014, 945},
    {"IsScreenRelativeMode", 78, 1, 0x0014, 946},
};

inline constexpr Field kFields_0123[] = {
    {"Flags", 0, 1, 0x0204, 414},
    {"ShadowThreshold", 8, 0, 0x002C, 229},
    {"ShadowDelay", 48, 0, 0x002C, 229},
    {"ShadowMoveAmount", 88, 0, 0x002C, 229},
};

inline constexpr Field kFields_0124[] = {
    {"Initial", 88, 4, 0x0008, 3939},
    {"SegmentSize", 96, 0, 0x002C, 229},
    {"Regen", 136, 8, 0x001C, 288},
    {"DisplayBar", 144, 8, 0x001C, 290},
    {"Shadow", 152, 8, 0x001C, 291},
};

inline constexpr Field kFields_0125[] = {
    {"TextColor", 0, 0, 0x002C, 1},
    {"BarColor", 16, 0, 0x002C, 1},
    {"BackgroundColor", 32, 0, 0x002C, 1},
    {"FullBarColor", 48, 0, 0x002C, 1},
    {"ThresholdColor", 64, 0, 0x002C, 1},
    {"DecrementColor", 80, 0, 0x002C, 1},
    {"Name", 96, 8, 0x0010, 0},
    {"ThresholdAmount", 104, 4, 0x0008, 3970},
    {"DecrementTimer", 108, 4, 0x0008, 3971},
    {"Width", 112, 4, 0x0008, 3972},
    {"Height", 116, 4, 0x0008, 3973},
    {"PositionX", 120, 4, 0x0008, 3974},
    {"PositionY", 124, 4, 0x0008, 3975},
    {"DisplayDuration", 128, 4, 0x0008, 3976},
    {"DisplayOption", 132, 1, 0x0104, 433},
    {"DisplayName", 133, 1, 0x0014, 947},
};

inline constexpr Field kFields_0126[] = {
    {"Scale", 0, 0, 0x002C, 6},
    {"FX", 8, 8, 0x0018, 0},
    {"Joint", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0127[] = {
    {"Fx", 0, 8, 0x001C, 294},
    {"Handle", 8, 4, 0x0000, 244},
};

inline constexpr Field kFields_0128[] = {
    {"HeapSize", 0, 4, 0x0000, 245},
    {"VramSize", 4, 4, 0x0000, 246},
    {"Slots", 8, 12, 0x0024, 139},
};

inline constexpr Field kFields_0129[] = {
    {"GroundImpulse", 0, 0, 0x002C, 11},
    {"AirImpulse", 14, 0, 0x002C, 11},
    {"BlockImpulse", 28, 0, 0x002C, 11},
    {"HitContext", 42, 2, 0x0000, 247},
    {"Scale", 44, 4, 0x0008, 3995},
    {"Name", 48, 8, 0x0018, 62},
    {"HitName", 56, 8, 0x0018, 63},
    {"DeathName", 64, 8, 0x0018, 64},
    {"SpawnJoint", 72, 8, 0x0010, 65},
    {"HitFlags", 80, 8, 0x0204, 443},
    {"MaxTime", 88, 4, 0x0008, 3996},
    {"MaxDamage", 92, 4, 0x0008, 3997},
    {"DamagePerHit", 96, 4, 0x0008, 3998},
    {"StunPerHit", 100, 4, 0x0008, 3999},
    {"MaxVelocity", 104, 4, 0x0008, 4000},
    {"HomingAmount", 108, 4, 0x0008, 4001},
    {"NumSouls", 112, 1, 0x0000, 248},
    {"HitsUntilSwitchMin", 113, 1, 0x0000, 249},
    {"HitsUntilSwitchMax", 114, 1, 0x0000, 250},
};

inline constexpr Field kFields_012A[] = {
    {"List", 0, 12, 0x0024, 140},
    {"Reach", 12, 4, 0x0008, 4002},
    {"MaxSouls", 16, 1, 0x0000, 251},
    {"MaxSoulsPerSecond", 17, 1, 0x0000, 252},
    {"Flags", 18, 1, 0x0204, 444},
};

inline constexpr Field kFields_012B[] = {
    {"Reach", 0, 4, 0x0008, 4003},
    {"TopRadius", 4, 4, 0x0008, 4004},
    {"BaseRadius", 8, 4, 0x0008, 4005},
};

inline constexpr Field kFields_012C[] = {
    {"BarName", 0, 8, 0x0010, 0},
    {"BarBudget", 8, 4, 0x0008, 4006},
};

inline constexpr Field kFields_012D[] = {
    {"TargetCreatureID", 0, 8, 0x0010, 0},
    {"TargetCreatureBlackboardVariable", 8, 8, 0x0010, 0},
    {"TargetPositionBlackboardVariable", 16, 8, 0x0010, 0},
    {"TargetCreatureJointName", 24, 8, 0x0010, 0},
    {"TargetRotationBlackboardVariable", 32, 8, 0x0010, 0},
    {"TargetPosition", 40, 0, 0x002C, 6},
    {"TargetCreatureJointOffset", 46, 0, 0x002C, 6},
    {"TargetRotation", 52, 0, 0x002C, 6},
    {"TargetTypeFlags", 58, 1, 0x0204, 445},
    {"TargetPositionSource", 59, 1, 0x0104, 446},
    {"TargetRotationSource", 60, 1, 0x0104, 447},
    {"TargetCreatureSource", 61, 1, 0x0104, 448},
};

inline constexpr Field kFields_012E[] = {
    {"MoodFlag", 0, 4, 0x0000, 253},
    {"MoodIndex", 4, 4, 0x0000, 254},
    {"MoodName", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_012F[] = {
    {"WeaponType", 0, 4, 0x0000, 255},
    {"WeaponName", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0130[] = {
    {"PromptInfo", 0, 0, 0x002C, 1459},
    {"PadDirection", 28, 4, 0x0008, 4024},
    {"SnapPoints", 32, 12, 0x0024, 141},
    {"PadAngle", 44, 4, 0x0008, 4025},
    {"ConnectCategories", 48, 12, 0x0024, 142},
    {"WeaponSwitchPercentage", 60, 4, 0x0008, 4026},
    {"ValidCharacters", 64, 12, 0x0024, 143},
    {"Button", 76, 1, 0x0104, 450},
    {"DrivenByMoveSystem", 77, 1, 0x0014, 950},
    {"NavMeshLink", 78, 1, 0x0014, 951},
    {"UsingTurnAround", 79, 1, 0x0014, 952},
    {"TraverseLogicalName", 80, 8, 0x0018, 0},
    {"MoveForwardName", 88, 8, 0x0018, 0},
    {"MoveBackwardName", 96, 8, 0x0018, 0},
    {"MoveIdleName", 104, 8, 0x0018, 0},
    {"MoveTurnedAround", 112, 8, 0x0018, 0},
    {"Menu", 120, 8, 0x0018, 0},
    {"Category", 128, 8, 0x0018, 0},
    {"EntryDirection", 136, 0, 0x002C, 6},
    {"WallDirection", 142, 0, 0x002C, 6},
    {"InteractZoneOffset", 148, 0, 0x002C, 6},
    {"FlipPadOnEnter", 154, 1, 0x0014, 953},
    {"StriderHangTransition", 155, 1, 0x0014, 954},
    {"InputMode", 156, 1, 0x0104, 451},
};

inline constexpr Field kFields_0131[] = {
    {"PropType", 0, 8, 0x0010, 0},
    {"PropSpawnGOName", 8, 8, 0x0010, 0},
    {"HandleJointRootName", 16, 8, 0x0010, 66},
    {"SynchJointName", 24, 8, 0x0010, 67},
    {"LootReward", 32, 8, 0x0010, 0},
    {"CullDistance", 40, 4, 0x0008, 4036},
    {"MaxHandleDistanceXZ", 44, 4, 0x0008, 4037},
    {"MaxHandleDistanceY", 48, 4, 0x0008, 4038},
    {"MaxHandleAngle", 52, 4, 0x0008, 4039},
    {"SpawnStatic", 56, 1, 0x0014, 955},
    {"LockRotation", 57, 1, 0x0014, 956},
};

inline constexpr Field kFields_0132[] = {
    {"Position", 0, 0, 0x002C, 6},
    {"Side", 6, 1, 0x0104, 452},
};

inline constexpr Field kFields_0133[] = {
    {"SnapPoints", 0, 12, 0x0024, 144},
    {"HintDistanceOffset", 12, 4, 0x0008, 4043},
    {"MoveType", 16, 8, 0x0018, 0},
    {"ParentCategory", 24, 8, 0x0018, 0},
    {"ForwardMoveName", 32, 8, 0x0018, 0},
    {"BackwardMoveName", 40, 8, 0x0018, 0},
    {"PromptOffset", 48, 0, 0x002C, 6},
    {"PromptOffsetPercent", 54, 0, 0x002C, 6},
    {"PromptOffsetBackward", 60, 0, 0x002C, 6},
    {"PromptOffsetBackwardPercent", 66, 0, 0x002C, 6},
    {"PromptOffscreenDistance", 72, 4, 0x0008, 4056},
    {"PromptOffscreenHintDistance", 76, 4, 0x0008, 4057},
    {"TraverseFilter", 80, 2, 0x0204, 453},
    {"DefaultDistance", 82, 2, 0x0008, 4058},
    {"DefaultHeight", 84, 2, 0x0008, 4059},
    {"DefaultXOffset", 86, 2, 0x0008, 4060},
    {"DefaultStartTLWidth", 88, 2, 0x0008, 4061},
    {"DefaultEndTLWidth", 90, 2, 0x0008, 4062},
    {"ForwardStartOffset", 92, 2, 0x0008, 4063},
    {"ForwardEndOffset", 94, 2, 0x0008, 4064},
    {"BackwardStartOffset", 96, 2, 0x0008, 4065},
    {"BackwardEndOffset", 98, 2, 0x0008, 4066},
    {"ZoneCenterZOffset", 100, 2, 0x0008, 4067},
    {"ZoneCenterYOffset", 102, 2, 0x0008, 4068},
    {"BoxZoneDepth", 104, 2, 0x0008, 4069},
    {"BoxZoneHeight", 106, 2, 0x0008, 4070},
    {"CylinderZoneRadius", 108, 2, 0x0008, 4071},
    {"CylinderZoneHeight", 110, 2, 0x0008, 4072},
    {"SweepAngleZoneAngle", 112, 2, 0x0008, 4073},
    {"SweepAngleZoneRadius", 114, 2, 0x0008, 4074},
    {"SweepAngleZoneHeight", 116, 2, 0x0008, 4075},
    {"SweepAngleZoneInnerRadius", 118, 2, 0x0008, 4076},
    {"SphereZoneRadius", 120, 2, 0x0008, 4077},
    {"MaxEntryAngle", 122, 2, 0x0008, 4078},
    {"MaxEntryCameraAngle", 124, 2, 0x0008, 4079},
    {"MaxDistanceXZ", 126, 2, 0x0008, 4080},
    {"MaxDistance3D", 128, 2, 0x0008, 4081},
    {"MaxHeight", 130, 2, 0x0008, 4082},
    {"MaxXOffset", 132, 2, 0x0008, 4083},
    {"MinDistanceXZ", 134, 2, 0x0008, 4084},
    {"MinDistance3D", 136, 2, 0x0008, 4085},
    {"MinHeight", 138, 2, 0x0008, 4086},
    {"MinVerticalAngle", 140, 2, 0x0008, 4087},
    {"MaxVerticalAngle", 142, 2, 0x0008, 4088},
    {"MaxHorizontalAngle", 144, 2, 0x0008, 4089},
    {"PromptWidthOffsetPercent", 146, 2, 0x0008, 4090},
    {"AIEdgeWarpOffset", 148, 2, 0x0008, 4091},
    {"TraverseType", 150, 1, 0x0104, 454},
    {"ZoneType", 151, 1, 0x0104, 455},
    {"ZoneCenterZDynamicOffset", 152, 1, 0x0104, 456},
    {"ZoneCenterYDynamicOffset", 153, 1, 0x0104, 457},
    {"OffsetZoneDimensionFromDynamicScale", 154, 1, 0x0014, 957},
    {"UseXZHintDistanceOffset", 155, 1, 0x0014, 958},
    {"DisableAfterUse", 156, 1, 0x0014, 959},
    {"PreventInteraction", 157, 1, 0x0014, 960},
    {"DefaultForceCompanionFollow", 158, 1, 0x0014, 961},
    {"AllowHintWithoutLOS", 159, 1, 0x0014, 962},
    {"ForwardPathing", 160, 1, 0x0014, 963},
    {"BackwardPathing", 161, 1, 0x0014, 964},
    {"DisableRotateExitZonePosition", 162, 1, 0x0014, 965},
    {"PromptOffsetUseHigherSideAsEnd", 163, 1, 0x0014, 966},
    {"PromptUseCustomBackwardOffset", 164, 1, 0x0014, 967},
    {"AIUseWithoutPath", 165, 1, 0x0014, 968},
    {"PromptWidthOffsetUseEnd", 166, 1, 0x0014, 969},
    {"DistanceDisplayFlags", 167, 1, 0x0204, 458},
};

inline constexpr Field kFields_0134[] = {
    {"Filter", 0, 8, 0x0018, 0},
    {"Type", 8, 1, 0x0104, 459},
};

inline constexpr Field kFields_0135[] = {
    {"Density", 0, 4, 0x0008, 4092},
    {"Reflectivity", 4, 4, 0x0008, 4093},
};

inline constexpr Field kFields_0136[] = {
    {"HitFlags", 0, 8, 0x0204, 460},
    {"Switch", 8, 2, 0x0000, 256},
};

inline constexpr Field kFields_0137[] = {
    {"FromSwitch", 0, 2, 0x0000, 257},
    {"ToSwitch", 2, 2, 0x0000, 258},
};

inline constexpr Field kFields_0138[] = {
    {"MaterialName", 0, 8, 0x0010, 0},
    {"AudioProperties", 8, 0, 0x002C, 309},
};

inline constexpr Field kFields_0139[] = {
    {"MFXSwitch", 0, 2, 0x0000, 259},
    {"PerJointCooldown", 2, 2, 0x0008, 4096},
    {"ImpulseThreshold", 4, 2, 0x0008, 4097},
};

inline constexpr Field kFields_013A[] = {
    {"MFXSwitchNamesMap", 0, 0, 0x0028, 6},
    {"MFXMaterialToIndicesMap", 16, 0, 0x0028, 7},
    {"MFXCharactersToCharacterTypes", 32, 0, 0x0028, 8},
    {"MFXWeaponToWeaponTypes", 48, 12, 0x0024, 145},
    {"MFXSwitchInheritanceList", 64, 12, 0x0024, 146},
    {"MFXAudioFilteringParameters", 80, 12, 0x0024, 147},
    {"MFXMaterialAudioProperties", 96, 12, 0x0024, 148},
    {"DefaultAudioFilteringParameters", 108, 0, 0x002C, 313},
};

inline constexpr Field kFields_013B[] = {
    {"Properties", 0, 12, 0x0024, 149},
};

inline constexpr Field kFields_013C[] = {
    {"DebugFlagStrings", 0, 12, 0x0024, 150},
};

inline constexpr Field kFields_013D[] = {
    {"HeightFieldModelInfo", 0, 12, 0x0024, 151},
};

inline constexpr Field kFields_013E[] = {
    {"Attributes", 0, 4096, 0x0024, 152},
    {"MPIconsScalingSettings", 4096, 0, 0x002C, 1469},
    {"PlayerStoneFreezeSpeeds", 4128, 0, 0x002C, 1468},
    {"AIStoneFreezeSpeeds", 4144, 0, 0x002C, 1468},
    {"PlayerIceFreezeSpeeds", 4160, 0, 0x002C, 1468},
    {"AIIceFreezeSpeeds", 4176, 0, 0x002C, 1468},
    {"AILifeCycleFreezeSpeeds", 4192, 0, 0x002C, 1468},
    {"Difficulty", 4208, 0, 0x002C, 1462},
    {"Player", 4224, 12, 0x0024, 154},
    {"WeaponGroupBare", 4236, 4, 0x0000, 264},
    {"ModeFlagData", 4240, 12, 0x0024, 155},
    {"DefaultDetonateWeaponType", 4252, 4, 0x0000, 265},
    {"PickupMap", 4256, 0, 0x0028, 9},
    {"TraversalOverrideInputControlAngleIn", 4268, 4, 0x0008, 4128},
    {"PickupSlotMap", 4272, 0, 0x0028, 10},
    {"TraversalOverrideInputControlAngleOut", 4284, 4, 0x0008, 4129},
    {"FlagGroups", 4288, 12, 0x0024, 156},
    {"TraversalOverrideInputDepthAngle", 4300, 4, 0x0008, 4130},
    {"WeaponLookupMap", 4304, 0, 0x0028, 11},
    {"TraversalOverrideInputPlanarAngle", 4316, 4, 0x0008, 4131},
    {"WeaponSlots", 4320, 12, 0x0024, 157},
    {"TraversalEnterInputDeadZone", 4332, 4, 0x0008, 4132},
    {"WeaponTypes", 4336, 12, 0x0024, 158},
    {"TraversalEnterInputSharpTurnThreshold", 4348, 4, 0x0008, 4133},
    {"WeaponGroups", 4352, 12, 0x0024, 159},
    {"TraversalDistanceToNodeDiffScale_Follow", 4364, 4, 0x0008, 4134},
    {"MoodLookupMap", 4368, 0, 0x0028, 12},
    {"TraversalDistanceToNodeDiffScale_NonFollow", 4380, 4, 0x0008, 4135},
    {"TraversePathLookupMap", 4384, 0, 0x0028, 13},
    {"RagdollTimeToCheckPartHitArrow", 4396, 4, 0x0008, 4136},
    {"TraverseGraphMoveFilterMap", 4400, 12, 0x0024, 160},
    {"RagdollTimeToCheckPartHitThrowable", 4412, 4, 0x0008, 4137},
    {"PropTypeLookupMap", 4416, 0, 0x0028, 14},
    {"RagdollTimeToCheckPartHitConcussion", 4428, 4, 0x0008, 4138},
    {"TraverseLinkMap", 4432, 0, 0x0028, 15},
    {"RagdollTimeToCheckPartHitOther", 4444, 4, 0x0008, 4139},
    {"ThrowableOnHitMap", 4448, 0, 0x0028, 16},
    {"TraverseLinkMinDistanceBetweenConnections", 4460, 4, 0x0008, 4140},
    {"ThrowableResponseMap", 4464, 0, 0x0028, 17},
    {"TraverseLinkMinDistanceToEdge", 4476, 4, 0x0008, 4141},
    {"UpgradeAwards", 4480, 12, 0x0024, 161},
    {"TraverseLinkRotateFacingAngle", 4492, 4, 0x0008, 4142},
    {"ContextNames", 4496, 12, 0x0024, 162},
    {"ReticleRadius", 4508, 4, 0x0008, 4143},
    {"Swirl", 4512, 12, 0x0024, 163},
    {"MaxReticleLength", 4524, 4, 0x0008, 4144},
    {"MeterBar", 4528, 12, 0x0024, 164},
    {"TargetingRange", 4540, 4, 0x0008, 4145},
    {"RsxProfileBarInfos", 4544, 12, 0x0024, 165},
    {"ManualTargetingRange", 4556, 4, 0x0008, 4146},
    {"HealthBarDebuffDisplayData", 4560, 12, 0x0024, 166},
    {"InteractPromptRange", 4572, 4, 0x0008, 4147},
    {"WeaponTrails", 4576, 12, 0x0024, 167},
    {"DeathMenuTime", 4588, 4, 0x0008, 4148},
    {"Varint", 4592, 12, 0x0024, 168},
    {"MeterBarHoldTime", 4604, 4, 0x0008, 4149},
    {"Varbool", 4608, 12, 0x0024, 169},
    {"FirstTimeMsgTimeOutDays", 4620, 4, 0x0008, 4150},
    {"Varfloat", 4624, 12, 0x0024, 170},
    {"DoublePressEventWindow", 4636, 4, 0x0008, 4151},
    {"Varstring", 4640, 12, 0x0024, 171},
    {"DoubleTapEventWindow", 4652, 4, 0x0008, 4152},
    {"PlayerCounter", 4656, 12, 0x0024, 172},
    {"WiggleALittleThreshold", 4668, 4, 0x0008, 4153},
    {"AccessibilityHighlightColors", 4672, 12, 0x0024, 173},
    {"WiggleALotThreshold", 4684, 4, 0x0008, 4154},
    {"ValhallaLevelsToSave", 4688, 12, 0x0024, 174},
    {"WiggleAFuckLotThreshold", 4700, 4, 0x0008, 4155},
    {"PersistentCreature", 4704, 12, 0x0024, 175},
    {"WiggleMeterDrain", 4716, 4, 0x0008, 4156},
    {"TeamSettings", 4720, 8, 0x001C, 1465},
    {"PlayerMemory", 4728, 8, 0x001C, 296},
    {"TraversePathSynchReferenceGOName", 4736, 8, 0x0010, 68},
    {"TraversalSynchReferenceHintIconName", 4744, 8, 0x0010, 69},
    {"TraversalSynchReferenceUnavailableIconName", 4752, 8, 0x0010, 70},
    {"DefaultReticleName", 4760, 8, 0x0010, 71},
    {"DefaultPaintTargetReticle", 4768, 8, 0x0010, 0},
    {"SaveBufferConfig", 4776, 0, 0x002C, 1461},
    {"MPIconsScreenRangeSettings", 4784, 8, 0x001C, 388},
    {"ValhallaEntitlementName", 4792, 8, 0x0018, 72},
    {"ValhallaStartingLevel", 4800, 8, 0x0018, 73},
    {"PersistentHaptic", 4808, 8, 0x001C, 481},
    {"RageModeWhenExpression", 4816, 8, 0x0030, 65535},
    {"MinVaultDistance", 4824, 4, 0x0008, 4157},
    {"MaxVaultDistance", 4828, 4, 0x0008, 4158},
    {"MaxJumpHeight", 4832, 4, 0x0008, 4159},
    {"JumpLandDistFromEdge", 4836, 4, 0x0008, 4160},
    {"GapTestOffset", 4840, 4, 0x0008, 4161},
    {"GapTestHeight", 4844, 4, 0x0008, 4162},
    {"GapJumpMinDist", 4848, 4, 0x0008, 4163},
    {"GapJumpMidDist", 4852, 4, 0x0008, 4164},
    {"GapJumpMaxDist", 4856, 4, 0x0008, 4165},
    {"AxeStuckTag", 4860, 4, 0x0000, 269},
    {"HeadPartFlag", 4864, 4, 0x0000, 270},
    {"WarnApproachMinAngle", 4868, 4, 0x0008, 4166},
    {"WarnApproachTime", 4872, 4, 0x0008, 4167},
    {"MashALittleThreshold", 4876, 4, 0x0008, 4168},
    {"MashALotThreshold", 4880, 4, 0x0008, 4169},
    {"MashAFuckLotThreshold", 4884, 4, 0x0008, 4170},
    {"MashMeterDrain", 4888, 4, 0x0008, 4171},
    {"RubALittleThreshold", 4892, 4, 0x0008, 4172},
    {"RubALotThreshold", 4896, 4, 0x0008, 4173},
    {"RubAFuckLotThreshold", 4900, 4, 0x0008, 4174},
    {"RubMeterDrain", 4904, 4, 0x0008, 4175},
    {"HoldDownDelay", 4908, 4, 0x0008, 4176},
    {"UnlightSpeed", 4912, 4, 0x0008, 4177},
    {"ReticleSwitchOffDelay", 4916, 4, 0x0008, 4178},
    {"DampedControllerSpeed", 4920, 4, 0x0008, 4179},
    {"ClampMaxExclusionCylinderSpeed", 4924, 4, 0x0008, 4180},
    {"BladeCollisionTolerance", 4928, 4, 0x0008, 4181},
    {"TrailFalloff", 4932, 4, 0x0008, 4182},
    {"ControlTweenSafety", 4936, 4, 0x0008, 4183},
    {"ControlTweenSpeed", 4940, 4, 0x0008, 4184},
    {"ControlTweenThreshold", 4944, 4, 0x0008, 4185},
    {"ProfileVersion", 4948, 4, 0x0000, 271},
    {"AttachModeFlagPrimary", 4952, 4, 0x0000, 272},
    {"AccessibilityMetalnessClamp", 4956, 4, 0x0008, 4186},
    {"AccessibilityScatterDesaturation", 4960, 4, 0x0008, 4187},
    {"PlayerIncludeTraverselinkFilter", 4964, 2, 0x0204, 461},
    {"PlayerExcludeTraverselinkFilter", 4966, 2, 0x0204, 462},
    {"TraversalEnterInputMode", 4968, 1, 0x0104, 463},
    {"DeathsBeforeEasyOffer", 4969, 1, 0x0000, 273},
    {"StartStopFrameInputDelay", 4970, 1, 0x0000, 274},
    {"ControllerButtonDownThreshold", 4971, 1, 0x0004, 464},
    {"NewGamePlusShipped", 4972, 1, 0x0014, 970},
};

inline constexpr Field kFields_013F[] = {
    {"AutoSpawnCharacter", 0, 12, 0x0024, 176},
};

inline constexpr Field kFields_0140[] = {
    {"OscillateFreq", 0, 4, 0x0008, 4188},
    {"OscillateAmpl", 4, 4, 0x0008, 4189},
    {"OscillateYOffset", 8, 4, 0x0008, 4190},
    {"LifeTime", 12, 4, 0x0008, 4191},
    {"LifeTimeVariance", 16, 4, 0x0008, 4192},
    {"SpawnTime", 20, 4, 0x0008, 4193},
    {"SpawnTimeVariance", 24, 4, 0x0008, 4194},
    {"AttractTime", 28, 4, 0x0008, 4195},
    {"AttractRadius", 32, 4, 0x0008, 4196},
    {"Points", 36, 4, 0x0008, 4197},
    {"Gravity", 40, 4, 0x0008, 4198},
    {"PickupSnd", 48, 8, 0x0018, 74},
    {"GOName", 56, 8, 0x0018, 75},
    {"OrbType", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_0141[] = {
    {"Orb", 0, 8, 0x001C, 320},
    {"EmitterJoint", 8, 8, 0x0018, 0},
    {"TargetJoint", 16, 8, 0x0018, 0},
    {"AttractJoint", 24, 8, 0x0018, 76},
    {"EmitterRadius", 32, 4, 0x0008, 4199},
    {"TargetYOffset", 36, 4, 0x0008, 4200},
    {"TargetZOffset", 40, 4, 0x0008, 4201},
    {"TargetRadius", 44, 4, 0x0008, 4202},
    {"AttractYOffset", 48, 4, 0x0008, 4203},
    {"AttractZOffset", 52, 4, 0x0008, 4204},
    {"NormalHealth", 56, 0, 0x002C, 1475},
    {"HealthThreshold", 60, 4, 0x0008, 4206},
    {"LowHealth", 64, 0, 0x002C, 1475},
};

inline constexpr Field kFields_0142[] = {
    {"SlideSound", 0, 8, 0x0018, 77},
    {"GrabSound", 8, 8, 0x0018, 78},
    {"GrabEffect", 16, 8, 0x0018, 79},
    {"InitialGrabSound", 24, 8, 0x0018, 80},
    {"SwingSound", 32, 8, 0x0018, 81},
    {"SegmentMass", 40, 4, 0x0008, 4208},
    {"SpringConstant", 44, 4, 0x0008, 4209},
    {"SpringDamping", 48, 4, 0x0008, 4210},
    {"SpringConstant2", 52, 4, 0x0008, 4211},
    {"SpringDamping2", 56, 4, 0x0008, 4212},
    {"BraceSpringConstant", 60, 4, 0x0008, 4213},
    {"BraceSpringDamping", 64, 4, 0x0008, 4214},
    {"GlobalDamping", 68, 4, 0x0008, 4215},
    {"Gravity", 72, 4, 0x0008, 4216},
    {"OverSample", 76, 1, 0x0000, 279},
    {"RopeDirType", 77, 1, 0x0104, 465},
};

inline constexpr Field kFields_0143[] = {
    {"AudioInfo", 0, 8, 0x001C, 1476},
    {"MasterReplaceAnim", 8, 8, 0x0010, 0},
    {"TotalTime", 16, 4, 0x0008, 4217},
    {"Flags", 20, 2, 0x0204, 466},
    {"Event", 22, 1, 0x0104, 467},
};

inline constexpr Field kFields_0144[] = {
    {"SlipToleranceTime", 24, 4, 0x0008, 4219},
    {"QuickPenaltyTime", 28, 4, 0x0008, 4220},
    {"IdleStruggle", 32, 4, 0x0008, 4221},
    {"ForwardAmount", 36, 4, 0x0008, 4222},
    {"PushBackAmount", 40, 4, 0x0008, 4223},
    {"TugDelay", 44, 4, 0x0008, 4224},
    {"SuccessThreshold", 48, 4, 0x0008, 4225},
    {"AccesibilityHoldScale", 52, 4, 0x0008, 4226},
    {"Button", 56, 1, 0x0104, 470},
};

inline constexpr Field kFields_0145[] = {
    {"SlipTolerance", 24, 4, 0x0008, 4228},
    {"RewindSpeed", 28, 4, 0x0008, 4229},
    {"Button", 32, 1, 0x0104, 473},
};

inline constexpr Field kFields_0146[] = {
    {"OnSound", 0, 8, 0x0010, 0},
    {"PressSound", 8, 8, 0x0010, 0},
    {"Button", 16, 1, 0x0104, 474},
    {"MaxWrongZones", 17, 1, 0x0000, 280},
    {"Flags", 18, 1, 0x0204, 475},
};

inline constexpr Field kFields_0147[] = {
    {"Threshold", 0, 4, 0x0008, 4230},
    {"CircleID", 4, 1, 0x0000, 281},
    {"Actions", 5, 1, 0x0204, 476},
    {"When", 8, 8, 0x0030, 65535},
};

inline constexpr Field kFields_0148[] = {
    {"Class", 0, 1, 0x0104, 477},
};

inline constexpr Field kFields_0149[] = {
    {"Radius", 4, 4, 0x0008, 4231},
    {"Height", 8, 4, 0x0008, 4232},
    {"MPIconOverrideHeight", 12, 4, 0x0008, 4233},
    {"WallPenetrationThreshold", 16, 4, 0x0008, 4234},
    {"BaseGravity", 20, 4, 0x0008, 4235},
    {"MinCombatDistance", 24, 4, 0x0008, 4236},
    {"PathingCutOutRadius", 28, 4, 0x0008, 4237},
    {"NavFlags", 32, 4, 0x0204, 479},
    {"HeadTrackJoint", 40, 8, 0x0018, 0},
};

inline constexpr Field kFields_014A[] = {
    {"JointName", 0, 8, 0x0010, 0},
    {"BoneOffset", 8, 4, 0x0008, 4238},
    {"priority", 12, 4, 0x0008, 4239},
};

inline constexpr Field kFields_014B[] = {
    {"TargetJointList", 0, 12, 0x0024, 177},
    {"MinAngle", 12, 4, 0x0008, 4240},
    {"StickJointList", 16, 12, 0x0024, 178},
    {"MaxAngle", 28, 4, 0x0008, 4241},
    {"DynamicJointList", 32, 12, 0x0024, 179},
    {"Name", 48, 8, 0x0010, 0},
};

inline constexpr Field kFields_014C[] = {
    {"ReferenceJointName", 0, 8, 0x0010, 0},
    {"ExposedSideInfoList", 8, 64, 0x0024, 180},
};

inline constexpr Field kFields_014D[] = {
    {"JiggleDistance", 0, 4, 0x0008, 4242},
    {"JiggleClamp", 4, 4, 0x0008, 4243},
};

inline constexpr Field kFields_014E[] = {
    {"JiggleAxis", 0, 1, 0x0104, 480},
    {"PositiveDirection", 4, 0, 0x002C, 333},
    {"NegativeDirection", 12, 0, 0x002C, 333},
};

inline constexpr Field kFields_014F[] = {
    {"ResponseAxisList", 0, 12, 0x0024, 181},
    {"ReactDuration", 12, 4, 0x0008, 4248},
    {"PartFlags", 16, 8, 0x0204, 481},
    {"JointToWiggle", 24, 8, 0x0010, 0},
    {"HoldDuration", 32, 4, 0x0008, 4249},
    {"PercentHoldStart", 36, 4, 0x0008, 4250},
    {"Ease", 40, 4, 0x0008, 4251},
    {"RotateFrequency", 44, 4, 0x0008, 4252},
    {"Scale", 48, 4, 0x0008, 4253},
    {"MaxDistanceFromJoint", 52, 4, 0x0008, 4254},
    {"CacheJointIndex", 56, 4, 0x0000, 282},
    {"EffectorIndex", 60, 1, 0x0104, 482},
};

inline constexpr Field kFields_0150[] = {
    {"ConstraintMaxDistance", 0, 4, 0x0008, 4255},
    {"ConstraintMinDistance", 4, 4, 0x0008, 4256},
    {"ConstraintMaxAngle", 8, 4, 0x0008, 4257},
};

inline constexpr Field kFields_0151[] = {
    {"PickupId", 0, 2, 0x0000, 283},
    {"PickupStage", 2, 1, 0x0000, 284},
};

inline constexpr Field kFields_0152[] = {
    {"ActivatePickup", 0, 12, 0x0024, 182},
    {"MinAmount", 12, 4, 0x0008, 4258},
    {"StatusMeterName", 16, 8, 0x0010, 0},
    {"pointer_2", 24, 8, 0x001C, 337},
    {"MaxAmount", 32, 4, 0x0008, 4259},
    {"StatusMeterThresholdFlags", 36, 1, 0x0204, 483},
};

inline constexpr Field kFields_0153[] = {
    {"DefaultVictimResistCombatFaceImpulseRedirect", 1, 1, 0x0014, 971},
    {"DefaultCanBouncePendulum", 2, 1, 0x0014, 972},
    {"DefaultCanHitPendulum", 3, 1, 0x0014, 973},
    {"AggressionRetentionTime", 4, 4, 0x0008, 4260},
    {"TargetInfo", 8, 0, 0x002C, 1477},
    {"LargeCreatureTargetingParams", 72, 0, 0x002C, 240},
    {"ExposedContextInfoMap", 96, 0, 0x0028, 18},
    {"DefaultAttackerCombatFaceImpulseRedirectPercent", 108, 4, 0x0008, 4271},
    {"StatusMeterThresholds", 112, 12, 0x0024, 185},
    {"DefaultAttackerCombatFaceImpulseRedirectRandomRange", 124, 4, 0x0008, 4272},
    {"HealthThresholdList", 128, 12, 0x0024, 186},
    {"HitStick", 140, 0, 0x002C, 336},
    {"IKHitDamageResponseList", 152, 12, 0x0024, 187},
    {"DefaultHitModifierList", 168, 12, 0x0024, 188},
    {"Flags", 184, 8, 0x0204, 485},
    {"DefaultExposedContext", 192, 8, 0x0010, 0},
};

inline constexpr Field kFields_0154[] = {
    {"QuestStartedEventName", 0, 8, 0x0018, 82},
    {"QuestCompletedEventName", 8, 8, 0x0018, 83},
    {"CharactersUpdateEventName", 16, 8, 0x0018, 84},
    {"LevelChangedEventName", 24, 8, 0x0018, 85},
    {"ActivityStartedEventName", 32, 8, 0x0018, 86},
    {"ActivityEndedEventName", 40, 8, 0x0018, 87},
    {"ActivityResumedEventName", 48, 8, 0x0018, 88},
    {"ActivityAvailabilityChangedEventName", 56, 8, 0x0018, 89},
    {"UDSActivities", 64, 12, 0x0024, 189},
};

inline constexpr Field kFields_0155[] = {
    {"Type", 0, 1, 0x0104, 486},
    {"DamageScale", 4, 4, 0x0008, 4276},
    {"AllowHit", 8, 1, 0x0014, 974},
    {"AllowHitPause", 9, 1, 0x0014, 975},
    {"AllowReaction", 10, 1, 0x0014, 976},
    {"AllowStun", 11, 1, 0x0014, 977},
    {"AllowImpulse", 12, 1, 0x0014, 978},
    {"AllowPickup", 13, 1, 0x0014, 979},
    {"AllowAttackerHit", 14, 1, 0x0014, 980},
    {"AllowMFX", 15, 1, 0x0014, 981},
};

inline constexpr Field kFields_0156[] = {
    {"HitModifierIfFailed", 16, 0, 0x002C, 341},
    {"FailAngles", 32, 12, 0x0024, 190},
    {"FailAngle", 44, 4, 0x0008, 4279},
};

inline constexpr Field kFields_0157[] = {
    {"EnemyContextList", 0, 12, 0x0024, 191},
    {"ExcludeActivationFlags", 12, 1, 0x0014, 998},
    {"ExcludePartFlags", 13, 1, 0x0014, 999},
    {"CheckAllParts", 14, 1, 0x0014, 1000},
    {"ExcludeEnemyContexts", 15, 1, 0x0014, 1001},
    {"EnemyIDList", 16, 12, 0x0024, 192},
    {"ExcludeEnemyIDs", 28, 1, 0x0014, 1002},
    {"ExcludeDynamicFlags", 29, 1, 0x0014, 1003},
    {"EnemyDynamicFlagFilterList", 32, 12, 0x0024, 193},
    {"ActivationFlags", 48, 8, 0x0204, 489},
    {"ExcludeHitFlags", 56, 8, 0x0204, 490},
    {"PartFlags", 64, 8, 0x0204, 491},
    {"HitModifier", 72, 8, 0x001C, 341},
};

inline constexpr Field kFields_0158[] = {
    {"InitialVelocityXMin", 0, 4, 0x0008, 4280},
    {"InitialVelocityXMax", 4, 4, 0x0008, 4281},
    {"InitialVelocityYMin", 8, 4, 0x0008, 4282},
    {"InitialVelocityYMax", 12, 4, 0x0008, 4283},
    {"InitialVelocityZMin", 16, 4, 0x0008, 4284},
    {"InitialVelocityZMax", 20, 4, 0x0008, 4285},
    {"InitialRotationalVelocity", 24, 4, 0x0008, 4286},
    {"InitialRotationDistribution", 28, 1, 0x0104, 492},
    {"DurationAfterRest", 32, 4, 0x0008, 4287},
    {"FadeOutTime", 36, 4, 0x0008, 4288},
    {"Gravity", 40, 4, 0x0008, 4289},
    {"Bounciness", 44, 4, 0x0008, 4290},
    {"FakeRotationalHitResponse", 48, 4, 0x0008, 4291},
    {"ForceRestingSpeed", 52, 4, 0x0008, 4292},
    {"ImpactSoundName", 56, 8, 0x0018, 90},
    {"MinImpactVolume", 64, 2, 0x0000, 285},
    {"MaxImpactVolume", 66, 2, 0x0000, 286},
    {"MaxImpactSpeed", 68, 4, 0x0008, 4293},
};

inline constexpr Field kFields_0159[] = {
    {"ProgressionFactor", 0, 1, 0x0104, 493},
    {"StartTime", 4, 4, 0x0008, 4294},
    {"Intensity", 8, 4, 0x0008, 4295},
    {"Oscillations", 12, 4, 0x0008, 4296},
};

inline constexpr Field kFields_015A[] = {
    {"ImmediateWindStrength", 0, 4, 0x0008, 4297},
    {"ImmediateWindRadius", 4, 4, 0x0008, 4298},
    {"FalloffWindStrength", 8, 4, 0x0008, 4299},
    {"FalloffWindTime", 12, 4, 0x0008, 4300},
    {"FalloffWindNoiseFreq", 16, 4, 0x0008, 4301},
    {"FalloffWindSeparation", 20, 4, 0x0008, 4302},
};

inline constexpr Field kFields_015B[] = {
    {"NavBank", 0, 8, 0x0018, 91},
    {"StrafeRange", 8, 4, 0x0008, 4303},
    {"Tags", 12, 4, 0x0204, 494},
};

inline constexpr Field kFields_015C[] = {
    {"TrackName", 0, 8, 0x0018, 0},
    {"TouchEvents", 8, 12, 0x0024, 194},
};

inline constexpr Field kFields_015D[] = {
    {"DeathEffects", 0, 12, 0x0024, 195},
    {"StoneFreezing", 16, 0, 0x002C, 1485},
    {"IceFreezing", 136, 0, 0x002C, 1485},
    {"LifeCycleFreezing", 256, 0, 0x002C, 1485},
};

inline constexpr Field kFields_015E[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"Cnt", 8, 2, 0x0004, 498},
};

inline constexpr Field kFields_015F[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"ElementCount", 8, 2, 0x0004, 499},
    {"CustomCountNameParam1", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0160[] = {
    {"Size", 0, 4, 0x0000, 290},
    {"Identifier", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0161[] = {
    {"ScriptName", 0, 8, 0x0010, 0},
    {"GOName", 8, 8, 0x0010, 0},
    {"Size", 16, 4, 0x0000, 291},
};

inline constexpr Field kFields_0162[] = {
    {"ClonePoseSkeletonMapBufferSize", 0, 4, 0x0000, 292},
    {"ClonePoseMatrixBufferSize", 4, 4, 0x0000, 293},
    {"MaxClonePoseTargets", 8, 4, 0x0000, 294},
    {"MaxClonePoseObjects", 12, 4, 0x0000, 295},
};

inline constexpr Field kFields_0163[] = {
    {"GOPool", 0, 12, 0x0024, 199},
    {"MemoryPools", 16, 12, 0x0024, 200},
    {"MemoryLua", 32, 12, 0x0024, 201},
    {"MemoryVisualScripting", 48, 12, 0x0024, 202},
    {"RequiredPickups", 64, 12, 0x0024, 203},
    {"DependentResourceWads", 80, 12, 0x0024, 204},
    {"MaxWeaponDropPerType", 92, 1, 0x0000, 297},
    {"MaxDynamicGOClients", 94, 2, 0x0004, 500},
    {"MemoryClonePose", 96, 0, 0x002C, 354},
    {"Varint", 112, 12, 0x0024, 205},
    {"Varbool", 128, 12, 0x0024, 206},
    {"Varfloat", 144, 12, 0x0024, 207},
    {"Varstring", 160, 12, 0x0024, 208},
};

inline constexpr Field kFields_0164[] = {
    {"Spread", 0, 4, 0x0008, 4322},
    {"SpreadFadePercent", 4, 4, 0x0008, 4323},
    {"Speed", 8, 4, 0x0008, 4324},
    {"NumSouls", 12, 1, 0x0000, 302},
    {"AttackDistance", 16, 4, 0x0008, 4325},
    {"AttackTime", 20, 4, 0x0008, 4326},
    {"AttackMinAngle", 24, 4, 0x0008, 4327},
    {"AttackSpeed", 28, 4, 0x0008, 4328},
    {"AttackTargetRadius", 32, 4, 0x0008, 4329},
    {"AttackBranch", 40, 8, 0x001C, 1286},
    {"AvoidDistance", 48, 4, 0x0008, 4330},
    {"AvoidTime", 52, 4, 0x0008, 4331},
    {"AvoidTargetRadius", 56, 4, 0x0008, 4332},
    {"AvoidSpeed", 60, 4, 0x0008, 4333},
    {"AvoidBranch", 64, 8, 0x001C, 1286},
};

inline constexpr Field kFields_0165[] = {
    {"Min", 0, 4, 0x0008, 4334},
    {"Max", 4, 4, 0x0008, 4335},
};

inline constexpr Field kFields_0166[] = {
    {"EnemyCreator", 0, 8, 0x0018, 0},
    {"RespawnWhileAlive", 8, 1, 0x0000, 303},
    {"Weight", 9, 1, 0x0000, 304},
};

inline constexpr Field kFields_0167[] = {
    {"SpawnPoint", 0, 12, 0x0024, 209},
    {"NavGraphWeight", 12, 4, 0x0008, 4336},
    {"ConfigSpecs", 16, 12, 0x0024, 210},
    {"Order", 28, 1, 0x0104, 501},
    {"StayOnNavGraph", 29, 1, 0x0000, 305},
    {"NavGraphCollection", 32, 8, 0x0010, 0},
    {"SpawnMove", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_0168[] = {
    {"Generator", 0, 8, 0x001C, 359},
    {"MinDistance", 8, 4, 0x0008, 4337},
    {"MaxDistance", 12, 4, 0x0008, 4338},
    {"Type", 16, 1, 0x0104, 502},
    {"Weight", 17, 1, 0x0000, 306},
};

inline constexpr Field kFields_0169[] = {
    {"Creature", 0, 12, 0x0024, 211},
    {"MarkerID", 12, 2, 0x0000, 307},
    {"TotalCount", 14, 2, 0x0000, 308},
    {"Spawn", 16, 12, 0x0024, 212},
    {"InitSpawnDelay", 28, 0, 0x002C, 357},
    {"RespawnDelay", 36, 0, 0x002C, 357},
    {"WaveTimer", 44, 0, 0x002C, 357},
    {"TriggerNextOnKills", 52, 2, 0x0000, 309},
    {"InitCount", 54, 1, 0x0000, 310},
    {"MinCount", 55, 1, 0x0000, 311},
    {"MaxCount", 56, 1, 0x0000, 312},
    {"IncCount", 57, 1, 0x0000, 313},
};

inline constexpr Field kFields_016A[] = {
    {"Element", 0, 12, 0x0024, 213},
    {"Order", 12, 1, 0x0104, 503},
};

inline constexpr Field kFields_016B[] = {
    {"Tint", 0, 0, 0x002C, 1},
    {"ShadowTint", 16, 0, 0x002C, 1},
    {"ScaledSizes", 32, 12, 0x0024, 214},
    {"Size", 44, 4, 0x0008, 4354},
    {"ScaledParagraphIndents", 48, 12, 0x0024, 215},
    {"CharSpacing", 60, 4, 0x0008, 4356},
    {"ScaledLeftMargins", 64, 12, 0x0024, 216},
    {"WordSpacing", 76, 4, 0x0008, 4358},
    {"ScaledRightMargins", 80, 12, 0x0024, 217},
    {"LineSpacing", 92, 4, 0x0008, 4360},
    {"TemplateSymbol", 96, 8, 0x001A, 0},
    {"Font", 104, 8, 0x0010, 0},
    {"AlternativeFont", 112, 8, 0x0010, 0},
    {"FixedWidth", 120, 4, 0x0008, 4361},
    {"Stretch", 124, 4, 0x0008, 4362},
    {"Slant", 128, 4, 0x0008, 4363},
    {"ParagraphSpacing", 132, 4, 0x0008, 4364},
    {"ParagraphIndent", 136, 4, 0x0008, 4365},
    {"LeftMargin", 140, 4, 0x0008, 4366},
    {"RightMargin", 144, 4, 0x0008, 4367},
    {"ShadowX", 148, 4, 0x0008, 4368},
    {"ShadowY", 152, 4, 0x0008, 4369},
    {"Curvature", 156, 4, 0x0008, 4370},
    {"TabWidth", 160, 4, 0x0008, 4371},
    {"Font_IsNull", 164, 1, 0x0016, 1004},
    {"AlternativeFont_IsNull", 165, 1, 0x0016, 1005},
    {"Tint_IsNull", 166, 1, 0x0016, 1006},
    {"Size_IsNull", 167, 1, 0x0016, 1007},
    {"ScaledSizes_IsNull", 168, 1, 0x0016, 1008},
    {"CharSpacing_IsNull", 169, 1, 0x0016, 1009},
    {"WordSpacing_IsNull", 170, 1, 0x0016, 1010},
    {"LineSpacing_IsNull", 171, 1, 0x0016, 1011},
    {"FixedWidth_IsNull", 172, 1, 0x0016, 1012},
    {"Stretch_IsNull", 173, 1, 0x0016, 1013},
    {"Slant_IsNull", 174, 1, 0x0016, 1014},
    {"ParagraphSpacing_IsNull", 175, 1, 0x0016, 1015},
    {"ParagraphIndent_IsNull", 176, 1, 0x0016, 1016},
    {"ScaledParagraphIndents_IsNull", 177, 1, 0x0016, 1017},
    {"LeftMargin_IsNull", 178, 1, 0x0016, 1018},
    {"ScaledLeftMargins_IsNull", 179, 1, 0x0016, 1019},
    {"RightMargin_IsNull", 180, 1, 0x0016, 1020},
    {"ScaledRightMargins_IsNull", 181, 1, 0x0016, 1021},
    {"IndentFirstLine", 182, 1, 0x0014, 1022},
    {"IndentFirstLine_IsNull", 183, 1, 0x0016, 1023},
    {"Alignment", 184, 1, 0x0104, 504},
    {"Alignment_IsNull", 185, 1, 0x0016, 1024},
    {"VertAlignment", 186, 1, 0x0104, 505},
    {"VertAlignment_IsNull", 187, 1, 0x0016, 1025},
    {"CaseTransform", 188, 1, 0x0104, 506},
    {"CaseTransform_IsNull", 189, 1, 0x0016, 1026},
    {"SmallCaps", 190, 1, 0x0014, 1027},
    {"SmallCaps_IsNull", 191, 1, 0x0016, 1028},
    {"Kerning", 192, 1, 0x0014, 1029},
    {"Kerning_IsNull", 193, 1, 0x0016, 1030},
    {"ShadowTint_IsNull", 194, 1, 0x0016, 1031},
    {"ShadowX_IsNull", 195, 1, 0x0016, 1032},
    {"ShadowY_IsNull", 196, 1, 0x0016, 1033},
    {"Curvature_IsNull", 197, 1, 0x0016, 1034},
    {"TabWidth_IsNull", 198, 1, 0x0016, 1035},
    {"Bold", 199, 1, 0x0014, 1036},
    {"Bold_IsNull", 200, 1, 0x0016, 1037},
};

inline constexpr Field kFields_016C[] = {
    {"MudDecayRate", 0, 4, 0x0008, 4372},
    {"SnowDecayRate", 4, 4, 0x0008, 4373},
    {"BloodDecayRate", 8, 4, 0x0008, 4374},
    {"FireDecayRate", 12, 4, 0x0008, 4375},
    {"FrostDecayRate", 16, 4, 0x0008, 4376},
    {"WetnessDecayRate", 20, 4, 0x0008, 4377},
    {"PoisonDecayRate", 24, 4, 0x0008, 4378},
    {"MudGamma", 28, 4, 0x0008, 4379},
    {"SnowGamma", 32, 4, 0x0008, 4380},
    {"BloodGamma", 36, 4, 0x0008, 4381},
    {"FireGamma", 40, 4, 0x0008, 4382},
    {"FrostGamma", 44, 4, 0x0008, 4383},
    {"WetnessGamma", 48, 4, 0x0008, 4384},
    {"PoisonGamma", 52, 4, 0x0008, 4385},
    {"BloodDecayInCombat", 56, 1, 0x0014, 1038},
};

inline constexpr Field kFields_016D[] = {
    {"JointName", 0, 8, 0x0010, 0},
    {"FrontLeftRegionID", 8, 4, 0x0000, 314},
    {"FrontRightRegionID", 12, 4, 0x0000, 315},
    {"BackLeftRegionID", 16, 4, 0x0000, 316},
    {"BackRightRegionID", 20, 4, 0x0000, 317},
};

inline constexpr Field kFields_016E[] = {
    {"JointMappings", 0, 12, 0x0024, 218},
};

inline constexpr Field kFields_016F[] = {
    {"RegionIdA", 0, 4, 0x0000, 318},
    {"RegionIdB", 4, 4, 0x0000, 319},
    {"Weight", 8, 4, 0x0008, 4386},
};

inline constexpr Field kFields_0170[] = {
    {"Neighbors", 0, 12, 0x0024, 219},
};

inline constexpr Field kFields_0171[] = {
    {"SlicesX", 0, 1, 0x0004, 507},
    {"SlicesY", 1, 1, 0x0004, 508},
    {"SlicesZ", 2, 1, 0x0004, 509},
    {"SizeX", 4, 4, 0x0008, 4387},
    {"SizeY", 8, 4, 0x0008, 4388},
    {"SizeZ", 12, 4, 0x0008, 4389},
};

inline constexpr Field kFields_0172[] = {
    {"IconName", 0, 8, 0x0010, 0},
    {"RadiusIconName", 8, 8, 0x0010, 0},
    {"InWorld_tMPIcon_Name", 16, 8, 0x0010, 0},
    {"IconScale", 24, 4, 0x0008, 4390},
    {"IsMainQuest", 28, 1, 0x0014, 1039},
};

inline constexpr Field kFields_0173[] = {
    {"MainQuestDistanceLabelNames", 0, 80, 0x0024, 220},
    {"SideQuestDistanceLabelNames", 80, 80, 0x0024, 221},
    {"MainQuestDistanceLabelUiTextVarNames", 160, 80, 0x0024, 222},
    {"SideQuestDistanceLabelUiTextVarNames", 240, 80, 0x0024, 223},
    {"AdvanceRadiusMultipliers", 320, 0, 0x002C, 1486},
    {"LerpSpeedMultipliers", 348, 0, 0x002C, 1486},
    {"CurrentPathBias", 376, 0, 0x002C, 1486},
    {"CurrentPathBiasFalloff", 404, 0, 0x002C, 1486},
    {"RealmNorthDirection", 432, 12, 0x0024, 224},
    {"LeftArrowIconScale", 444, 4, 0x0008, 4419},
    {"AlwaysLoadedRegions", 448, 12, 0x0024, 225},
    {"RightArrowIconScale", 460, 4, 0x0008, 4420},
    {"LeftArrowIconName", 464, 8, 0x0010, 104},
    {"RightArrowIconName", 472, 8, 0x0010, 105},
    {"DefaultIconClass", 480, 8, 0x0010, 0},
    {"InWorldMarkerIconPlacerReferenceGOName", 488, 8, 0x0010, 106},
    {"DistanceLabelUiTextMachineName", 496, 8, 0x0018, 107},
    {"NorthIconName", 504, 8, 0x0010, 108},
    {"SouthIconName", 512, 8, 0x0010, 109},
    {"WestIconName", 520, 8, 0x0010, 110},
    {"EastIconName", 528, 8, 0x0010, 111},
    {"DirectionTickIconName", 536, 8, 0x0010, 112},
    {"VerticalUpIconName", 544, 8, 0x0010, 113},
    {"VerticalDownIconName", 552, 8, 0x0010, 114},
    {"CentralRealmTravelPoint", 560, 8, 0x0010, 0},
    {"NorthDirection", 568, 0, 0x002C, 6},
    {"ScreenspacePosition", 574, 0, 0x002C, 6},
    {"InWorldMarkerIconsVisibleWithinRadius", 580, 4, 0x0008, 4427},
    {"TargetAdvanceRadius", 584, 4, 0x0008, 4428},
    {"TargetPositionLerpSpeed", 588, 4, 0x0008, 4429},
    {"PlayerStandingThreshold", 592, 4, 0x0008, 4430},
    {"TargetPositionCloseRadius", 596, 4, 0x0008, 4431},
    {"FuzzRadius", 600, 4, 0x0008, 4432},
    {"StraightPathDetectionRadius", 604, 4, 0x0008, 4433},
    {"StraightPathDetectionRadiusMultAt100m", 608, 4, 0x0008, 4434},
    {"MainQuestDistanceLabelScale", 612, 4, 0x0008, 4435},
    {"SideQuestDistanceLabelScale", 616, 4, 0x0008, 4436},
    {"MainQuestDistanceLabelVertOffset", 620, 4, 0x0008, 4437},
    {"SideQuestDistanceLabelVertOffset", 624, 4, 0x0008, 4438},
    {"NorthIconScale", 628, 4, 0x0008, 4439},
    {"SouthIconScale", 632, 4, 0x0008, 4440},
    {"WestIconScale", 636, 4, 0x0008, 4441},
    {"EastIconScale", 640, 4, 0x0008, 4442},
    {"DirectionTickIconScale", 644, 4, 0x0008, 4443},
    {"CardinalDirectionScreenspaceVertOffset", 648, 4, 0x0008, 4444},
    {"CardinalDirectionTickScreenspaceVertOffset", 652, 4, 0x0008, 4445},
    {"ScreenspaceWidth", 656, 4, 0x0008, 4446},
    {"Scale", 660, 4, 0x0008, 4447},
    {"Curvature", 664, 4, 0x0008, 4448},
    {"FOV", 668, 4, 0x0008, 4449},
    {"ZDelta", 672, 4, 0x0008, 4450},
    {"EndStackYOffset", 676, 4, 0x0008, 4451},
    {"EndStackYOffsetMult", 680, 4, 0x0008, 4452},
    {"EndStackArrowHorizOffset", 684, 4, 0x0008, 4453},
    {"EndStackArrowVertOffset", 688, 4, 0x0008, 4454},
    {"HiddenWaypointReminderDistance", 692, 4, 0x0008, 4455},
    {"OnCompassXPenetration", 696, 4, 0x0008, 4456},
    {"NearWaypointDistance", 700, 4, 0x0008, 4457},
    {"FarWaypointDistance", 704, 4, 0x0008, 4458},
    {"FarWaypointScale", 708, 4, 0x0008, 4459},
    {"DebugDisplayFwdOffset", 712, 4, 0x0008, 4460},
    {"DebugDisplayVertOffset", 716, 4, 0x0008, 4461},
    {"VerticalUpIconOffset", 720, 0, 0x002C, 3},
    {"VerticalDownIconOffset", 724, 0, 0x002C, 3},
    {"VerticalUpIconScale", 728, 4, 0x0008, 4466},
    {"VerticalDownIconScale", 732, 4, 0x0008, 4467},
    {"VerticalMarkerMinDistance", 736, 4, 0x0008, 4468},
    {"VerticalMarkerMaxZXDistance", 740, 4, 0x0008, 4469},
    {"VerticalMarkerMaxAngleFromVertical", 744, 4, 0x0008, 4470},
    {"MaximumEdgeLengthForOpenArea", 748, 4, 0x0008, 4471},
    {"WarningDistanceFromEdge", 752, 4, 0x0008, 4472},
    {"FuzzType", 756, 1, 0x0204, 510},
    {"WantStraightPathDetection", 757, 1, 0x0014, 1040},
    {"WantStraightPathDetectionOnlyInBoat", 758, 1, 0x0014, 1041},
    {"WantStraightPathHeadingFilter", 759, 1, 0x0014, 1042},
    {"ShowMultipleDistances", 760, 1, 0x0014, 1043},
};

inline constexpr Field kFields_0174[] = {
    {"CandidateSets", 0, 12, 0x0024, 226},
    {"OnUseWorldCandidateSet", 16, 8, 0x0010, 0},
    {"DefaultCandidateSet", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0175[] = {
    {"Layers", 0, 0, 0x0028, 19},
};

inline constexpr Field kFields_0176[] = {
    {"CameraPosition", 0, 0, 0x002C, 7},
    {"Lods", 16, 12, 0x0024, 227},
    {"Textures", 32, 12, 0x0024, 228},
};

inline constexpr Field kFields_0177[] = {
    {"Destinations", 0, 12, 0x0024, 229},
};

inline constexpr Field kFields_0178[] = {
    {"FVFSType", 0, 1, 0x0105, 511},
    {"Name", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0179[] = {
    {"SubMenu", 16, 8, 0x001C, 379},
};

inline constexpr Field kFields_017A[] = {
    {"VFSCommand", 16, 1, 0x0104, 514},
    {"VFSOption", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_017B[] = {
    {"Title", 0, 8, 0x0018, 0},
    {"Items", 8, 12, 0x0024, 230},
};

inline constexpr Field kFields_017C[] = {
    {"RopeSegments", 0, 1, 0x0000, 320},
    {"EnableSupportConstraints", 1, 1, 0x0014, 1044},
    {"EnableSlowdownDuringIdle", 2, 1, 0x0014, 1045},
    {"OffsetToAttachPoint", 4, 4, 0x0008, 4476},
    {"ExpectedStretchPercentage", 8, 4, 0x0008, 4477},
    {"RopeConstraintBias", 12, 4, 0x0008, 4478},
    {"RopeDamping", 16, 4, 0x0008, 4479},
    {"RopeDampingDuringSlowdown", 20, 4, 0x0008, 4480},
    {"PendulumPushForceMagnitude", 24, 4, 0x0008, 4481},
    {"PendulumConstraintBias", 28, 4, 0x0008, 4482},
    {"PendulumConstraintDamping", 32, 4, 0x0008, 4483},
    {"PendulumConstraintStiffness", 36, 4, 0x0008, 4484},
    {"SidewaysDampingFactor", 40, 4, 0x0008, 4485},
    {"PendulumTorqueScale", 44, 4, 0x0008, 4486},
    {"AdditionalGravityOnPendulum", 48, 4, 0x0008, 4487},
    {"TimeToStartSlowdown", 52, 4, 0x0008, 4488},
    {"UpdatesPerSecond", 56, 2, 0x0000, 321},
    {"CallbackMotionMax", 60, 4, 0x0008, 4489},
    {"CallbackMotionMin", 64, 4, 0x0008, 4490},
};

inline constexpr Field kFields_017D[] = {
    {"WadName", 0, 8, 0x0010, 0},
    {"ExpireTimestamp", 8, 4, 0x0004, 515},
    {"InitialTimestamp", 12, 4, 0x0004, 516},
    {"AllowOverflowForSmoketest", 16, 1, 0x0014, 1046},
};

inline constexpr Field kFields_017E[] = {
    {"Wads", 0, 12, 0x0024, 231},
};

inline constexpr Field kFields_017F[] = {
    {"Table", 0, 12, 0x0024, 232},
    {"DefaultExpireDeltaDays", 12, 2, 0x0000, 322},
};

inline constexpr Field kFields_0180[] = {
    {"TemplateSymbol", 0, 8, 0x001A, 0},
    {"CollisionOffsetJoint", 8, 8, 0x0010, 115},
    {"MinSpeed", 16, 4, 0x0008, 4491},
    {"MaxSpeed", 20, 4, 0x0008, 4492},
    {"MaxSpeedBackwards", 24, 4, 0x0008, 4493},
    {"MaxSpeedSideways", 28, 4, 0x0008, 4494},
    {"Acceleration", 32, 4, 0x0008, 4495},
    {"Deceleration", 36, 4, 0x0008, 4496},
    {"RotationTime", 40, 4, 0x0008, 4497},
    {"GravityOverride", 44, 4, 0x0008, 4498},
    {"MinSpeedWhenWalkingOffLedges", 48, 4, 0x0008, 4499},
    {"RotationFromPad", 52, 4, 0x0008, 4500},
    {"RotationFromAnimation", 56, 4, 0x0008, 4501},
    {"RotationFromCamera", 60, 4, 0x0008, 4502},
    {"ScaleAnimTranslation", 64, 4, 0x0008, 4503},
    {"ScaleAnimTranslation_YOverride", 68, 4, 0x0008, 4504},
    {"EnvironmentCollisionRadius", 72, 4, 0x0008, 4505},
    {"CreatureCollisionRadius", 76, 4, 0x0008, 4506},
    {"AttackCollisionRadius", 80, 4, 0x0008, 4507},
    {"CollisionOffsetX", 84, 4, 0x0008, 4508},
    {"CollisionOffsetY", 88, 4, 0x0008, 4509},
    {"CollisionOffsetZ", 92, 4, 0x0008, 4510},
    {"BlendTime", 96, 4, 0x0008, 4511},
    {"HighestWalkableStep", 100, 4, 0x0008, 4512},
    {"MaxAllowedSlopeAngle", 104, 4, 0x0008, 4513},
    {"MinAttackerAngleForWallSlide", 108, 4, 0x0008, 4514},
    {"MovementPriority", 112, 4, 0x0000, 323},
    {"FloatHeight", 116, 4, 0x0008, 4515},
    {"FloatHeightMin", 120, 4, 0x0008, 4516},
    {"FloatHeightMaxSpeed", 124, 4, 0x0008, 4517},
    {"GradualStickToGroundSpeed", 128, 4, 0x0008, 4518},
    {"CreatureToGroundLinetestLength", 132, 4, 0x0008, 4519},
    {"DistanceFromGroundToAirState", 136, 4, 0x0008, 4520},
    {"DistanceFromAirToGroundState", 140, 4, 0x0008, 4521},
    {"StickToWallAngle", 144, 4, 0x0008, 4522},
    {"HighFidelityLargeCreatureRadius", 148, 4, 0x0008, 4523},
    {"HighFidelityLargeCreatureHeight", 152, 4, 0x0008, 4524},
    {"HighFidelityPushbackBiasAngle", 156, 4, 0x0008, 4525},
    {"InvalidAngleForWallCollisionInCombat", 160, 4, 0x0008, 4526},
    {"MinSpeed_IsNull", 164, 1, 0x0016, 1047},
    {"MaxSpeed_IsNull", 165, 1, 0x0016, 1048},
    {"MaxSpeedBackwards_IsNull", 166, 1, 0x0016, 1049},
    {"MaxSpeedSideways_IsNull", 167, 1, 0x0016, 1050},
    {"Acceleration_IsNull", 168, 1, 0x0016, 1051},
    {"Deceleration_IsNull", 169, 1, 0x0016, 1052},
    {"RotationTime_IsNull", 170, 1, 0x0016, 1053},
    {"GravityOverride_IsNull", 171, 1, 0x0016, 1054},
    {"MinSpeedWhenWalkingOffLedges_IsNull", 172, 1, 0x0016, 1055},
    {"AnimApplyRotationCompensation", 173, 1, 0x0014, 1056},
    {"AnimApplyRotationCompensation_IsNull", 174, 1, 0x0016, 1057},
    {"RotationFromPad_IsNull", 175, 1, 0x0016, 1058},
    {"RotationFromAnimation_IsNull", 176, 1, 0x0016, 1059},
    {"RotationFromCamera_IsNull", 177, 1, 0x0016, 1060},
    {"MovementDirFromPad", 178, 1, 0x0014, 1061},
    {"MovementDirFromPad_IsNull", 179, 1, 0x0016, 1062},
    {"MovementDirFromAnimation", 180, 1, 0x0014, 1063},
    {"MovementDirFromAnimation_IsNull", 181, 1, 0x0016, 1064},
    {"MovementDirFromPadOverride", 182, 1, 0x0204, 517},
    {"MovementDirFromPadOverride_IsNull", 183, 1, 0x0016, 1065},
    {"MovementDirFromAnimationOverride", 184, 1, 0x0204, 518},
    {"MovementDirFromAnimationOverride_IsNull", 185, 1, 0x0016, 1066},
    {"SpeedFromCode", 186, 1, 0x0014, 1067},
    {"SpeedFromCode_IsNull", 187, 1, 0x0016, 1068},
    {"SpeedFromAnim", 188, 1, 0x0014, 1069},
    {"SpeedFromAnim_IsNull", 189, 1, 0x0016, 1070},
    {"ScaleAnimTranslation_IsNull", 190, 1, 0x0016, 1071},
    {"ScaleAnimTranslation_YOverride_IsNull", 191, 1, 0x0016, 1072},
    {"LockPadInputToPreviousInput", 192, 1, 0x0014, 1073},
    {"LockPadInputToPreviousInput_IsNull", 193, 1, 0x0016, 1074},
    {"PreventMotionWarpMotionParamOverride", 194, 1, 0x0014, 1075},
    {"PreventMotionWarpMotionParamOverride_IsNull", 195, 1, 0x0016, 1076},
    {"PadRotationStick", 196, 1, 0x0104, 519},
    {"PadRotationStick_IsNull", 197, 1, 0x0016, 1077},
    {"ForcePadInputBinary", 198, 1, 0x0014, 1078},
    {"ForcePadInputBinary_IsNull", 199, 1, 0x0016, 1079},
    {"LimitPadMotionInAir", 200, 1, 0x0014, 1080},
    {"LimitPadMotionInAir_IsNull", 201, 1, 0x0016, 1081},
    {"CollidesWithEnvironment", 202, 1, 0x0014, 1082},
    {"CollidesWithEnvironment_IsNull", 203, 1, 0x0016, 1083},
    {"CollidesWithWater", 204, 1, 0x0014, 1084},
    {"CollidesWithWater_IsNull", 205, 1, 0x0016, 1085},
    {"CollidesWithWalls", 206, 1, 0x0014, 1086},
    {"CollidesWithWalls_IsNull", 207, 1, 0x0016, 1087},
    {"CollidesWithActors", 208, 1, 0x0014, 1088},
    {"CollidesWithActors_IsNull", 209, 1, 0x0016, 1089},
    {"CollidesWithVehicles", 210, 1, 0x0014, 1090},
    {"CollidesWithVehicles_IsNull", 211, 1, 0x0016, 1091},
    {"CollidesWithInvisibleBarrier", 212, 1, 0x0014, 1092},
    {"CollidesWithInvisibleBarrier_IsNull", 213, 1, 0x0016, 1093},
    {"CollidesWithFightSpaceObstacles", 214, 1, 0x0014, 1094},
    {"CollidesWithFightSpaceObstacles_IsNull", 215, 1, 0x0016, 1095},
    {"CollidesWithLedgeBarrier", 216, 1, 0x0014, 1096},
    {"CollidesWithLedgeBarrier_IsNull", 217, 1, 0x0016, 1097},
    {"CollidesWithCombatGuide", 218, 1, 0x0014, 1098},
    {"CollidesWithCombatGuide_IsNull", 219, 1, 0x0016, 1099},
    {"CollidesWithHighFidelityLargeCreatureBarrier", 220, 1, 0x0014, 1100},
    {"CollidesWithHighFidelityLargeCreatureBarrier_IsNull", 221, 1, 0x0016, 1101},
    {"EnvironmentCollisionRadius_IsNull", 222, 1, 0x0016, 1102},
    {"CreatureCollisionRadius_IsNull", 223, 1, 0x0016, 1103},
    {"AttackCollisionRadius_IsNull", 224, 1, 0x0016, 1104},
    {"CollisionOffsetX_IsNull", 225, 1, 0x0016, 1105},
    {"CollisionOffsetY_IsNull", 226, 1, 0x0016, 1106},
    {"CollisionOffsetZ_IsNull", 227, 1, 0x0016, 1107},
    {"CollisionOffsetJoint_IsNull", 228, 1, 0x0016, 1108},
    {"CollisionOffsetType", 229, 1, 0x0104, 520},
    {"CollisionOffsetType_IsNull", 230, 1, 0x0016, 1109},
    {"BlendTime_IsNull", 231, 1, 0x0016, 1110},
    {"HighestWalkableStep_IsNull", 232, 1, 0x0016, 1111},
    {"MaxAllowedSlopeAngle_IsNull", 233, 1, 0x0016, 1112},
    {"MinAttackerAngleForWallSlide_IsNull", 234, 1, 0x0016, 1113},
    {"MovementPriority_IsNull", 235, 1, 0x0016, 1114},
    {"AllowPushBackOnAttacker", 236, 1, 0x0014, 1115},
    {"AllowPushBackOnAttacker_IsNull", 237, 1, 0x0016, 1116},
    {"IgnoreLargeCreaturePriorityAdvantage", 238, 1, 0x0014, 1117},
    {"IgnoreLargeCreaturePriorityAdvantage_IsNull", 239, 1, 0x0016, 1118},
    {"SplitCollisionResolutionOnWin", 240, 1, 0x0014, 1119},
    {"SplitCollisionResolutionOnWin_IsNull", 241, 1, 0x0016, 1120},
    {"INT8_HACK_ReallyUnmovable", 242, 1, 0x0014, 1121},
    {"INT8_HACK_ReallyUnmovable_IsNull", 243, 1, 0x0016, 1122},
    {"FloatHeight_IsNull", 244, 1, 0x0016, 1123},
    {"FloatHeightMin_IsNull", 245, 1, 0x0016, 1124},
    {"FloatHeightMaxSpeed_IsNull", 246, 1, 0x0016, 1125},
    {"GroundSamplingModeforFloating", 247, 1, 0x0104, 521},
    {"GroundSamplingModeforFloating_IsNull", 248, 1, 0x0016, 1126},
    {"IsEvadeMove", 249, 1, 0x0014, 1127},
    {"IsEvadeMove_IsNull", 250, 1, 0x0016, 1128},
    {"ForceNoSlideAroundAttacker", 251, 1, 0x0014, 1129},
    {"ForceNoSlideAroundAttacker_IsNull", 252, 1, 0x0016, 1130},
    {"ForceNoSlideAroundTarget", 253, 1, 0x0014, 1131},
    {"ForceNoSlideAroundTarget_IsNull", 254, 1, 0x0016, 1132},
    {"ForceNoSlideAroundPlayer", 255, 1, 0x0014, 1133},
    {"ForceNoSlideAroundPlayer_IsNull", 256, 1, 0x0016, 1134},
    {"ForceNoSlideForTarget", 257, 1, 0x0014, 1135},
    {"ForceNoSlideForTarget_IsNull", 258, 1, 0x0016, 1136},
    {"StickToGround", 259, 1, 0x0014, 1137},
    {"StickToGround_IsNull", 260, 1, 0x0016, 1138},
    {"GradualStickToGround", 261, 1, 0x0014, 1139},
    {"GradualStickToGround_IsNull", 262, 1, 0x0016, 1140},
    {"GradualStickToGroundSpeed_IsNull", 263, 1, 0x0016, 1141},
    {"CreatureToGroundLinetestLength_IsNull", 264, 1, 0x0016, 1142},
    {"DistanceFromGroundToAirState_IsNull", 265, 1, 0x0016, 1143},
    {"DistanceFromAirToGroundState_IsNull", 266, 1, 0x0016, 1144},
    {"StickToWallAngle_IsNull", 267, 1, 0x0016, 1145},
    {"AttackMoveStoppedByHostileCreature", 268, 1, 0x0014, 1146},
    {"AttackMoveStoppedByHostileCreature_IsNull", 269, 1, 0x0016, 1147},
    {"HighFidelityLargeCreatureRadius_IsNull", 270, 1, 0x0016, 1148},
    {"HighFidelityLargeCreatureHeight_IsNull", 271, 1, 0x0016, 1149},
    {"HighFidelityPushbackBiasAngle_IsNull", 272, 1, 0x0016, 1150},
    {"HighFidelityLargeCreatureSlipperyToAttackers", 273, 1, 0x0014, 1151},
    {"HighFidelityLargeCreatureSlipperyToAttackers_IsNull", 274, 1, 0x0016, 1152},
    {"BiasCollisionExtractionForHighFidelityLargeCreatures", 275, 1, 0x0014, 1153},
    {"BiasCollisionExtractionForHighFidelityLargeCreatures_IsNull", 276, 1, 0x0016, 1154},
    {"UseMainCollisionCapsuleForAttackerResolution", 277, 1, 0x0014, 1155},
    {"UseMainCollisionCapsuleForAttackerResolution_IsNull", 278, 1, 0x0016, 1156},
    {"UseAllCapsulesForAttackerResolution", 279, 1, 0x0014, 1157},
    {"UseAllCapsulesForAttackerResolution_IsNull", 280, 1, 0x0016, 1158},
    {"ForceHighFidelityLargeCreatureSolve", 281, 1, 0x0014, 1159},
    {"ForceHighFidelityLargeCreatureSolve_IsNull", 282, 1, 0x0016, 1160},
    {"KeepVelocityIfAttacked", 283, 1, 0x0014, 1161},
    {"KeepVelocityIfAttacked_IsNull", 284, 1, 0x0016, 1162},
    {"InvalidAngleForWallCollisionInCombat_IsNull", 285, 1, 0x0016, 1163},
};

inline constexpr Field kFields_0181[] = {
    {"TemplateSymbol", 0, 8, 0x001A, 0},
    {"TargetSpeed", 8, 4, 0x0008, 4527},
    {"RotationSpeed", 12, 4, 0x0008, 4528},
    {"FacingRotationSpeed", 16, 4, 0x0008, 4529},
    {"RotationFromFocus", 20, 4, 0x0008, 4530},
    {"StopDistance", 24, 4, 0x0008, 4531},
    {"StartDistance", 28, 4, 0x0008, 4532},
    {"Acceleration", 32, 4, 0x0008, 4533},
    {"Deceleration", 36, 4, 0x0008, 4534},
    {"DecelerationDistance", 40, 4, 0x0008, 4535},
    {"FloatHeight", 44, 4, 0x0008, 4536},
    {"StraightPathAngle", 48, 4, 0x0008, 4537},
    {"StrafeRotationSpeedOverride", 52, 4, 0x0008, 4538},
    {"TargetSpeed_IsNull", 56, 1, 0x0016, 1164},
    {"RotationSpeed_IsNull", 57, 1, 0x0016, 1165},
    {"FacingRotationSpeed_IsNull", 58, 1, 0x0016, 1166},
    {"RotationFromFocus_IsNull", 59, 1, 0x0016, 1167},
    {"StopDistance_IsNull", 60, 1, 0x0016, 1168},
    {"StartDistance_IsNull", 61, 1, 0x0016, 1169},
    {"Acceleration_IsNull", 62, 1, 0x0016, 1170},
    {"Deceleration_IsNull", 63, 1, 0x0016, 1171},
    {"DecelerationDistance_IsNull", 64, 1, 0x0016, 1172},
    {"FloatHeight_IsNull", 65, 1, 0x0016, 1173},
    {"StraightPathAngle_IsNull", 66, 1, 0x0016, 1174},
    {"StrafeRotationSpeedOverride_IsNull", 67, 1, 0x0016, 1175},
    {"Strafe", 68, 1, 0x0014, 1176},
    {"Strafe_IsNull", 69, 1, 0x0016, 1177},
    {"StrafeProto", 70, 1, 0x0014, 1178},
    {"StrafeProto_IsNull", 71, 1, 0x0016, 1179},
    {"StrafeStart", 72, 1, 0x0014, 1180},
    {"StrafeStart_IsNull", 73, 1, 0x0016, 1181},
    {"StrafeIdle", 74, 1, 0x0014, 1182},
    {"StrafeIdle_IsNull", 75, 1, 0x0016, 1183},
    {"Stop", 76, 1, 0x0014, 1184},
    {"Stop_IsNull", 77, 1, 0x0016, 1185},
    {"DisableMoveDirInterpolation", 78, 1, 0x0014, 1186},
    {"DisableMoveDirInterpolation_IsNull", 79, 1, 0x0016, 1187},
    {"IgnoreMaxSpeed", 80, 1, 0x0014, 1188},
    {"IgnoreMaxSpeed_IsNull", 81, 1, 0x0016, 1189},
};

inline constexpr Field kFields_0182[] = {
    {"Branches", 0, 12, 0x0024, 233},
};

inline constexpr Field kFields_0183[] = {
    {"MotionMap", 0, 0, 0x0028, 20},
    {"Navigation", 16, 8, 0x001C, 386},
    {"NavigationMap", 24, 0, 0x0028, 21},
    {"Branches", 40, 0, 0x0028, 22},
};

inline constexpr Field kFields_0184[] = {
    {"HalfWidthLeft", 0, 4, 0x0008, 4539},
    {"HalfWidthRight", 4, 4, 0x0008, 4540},
    {"HalfHeightTop", 8, 4, 0x0008, 4541},
    {"HalfHeightBottom", 12, 4, 0x0008, 4542},
    {"CornerRadius", 16, 4, 0x0008, 4543},
    {"CornerScaleX", 20, 4, 0x0008, 4544},
};

inline constexpr Field kFields_0185[] = {
    {"TweakName", 24, 8, 0x0010, 0},
    {"JointName", 32, 8, 0x0010, 0},
    {"Action", 40, 1, 0x0104, 527},
};

inline constexpr Field kFields_0186[] = {
    {"SlotsList", 24, 12, 0x0024, 234},
    {"Action", 36, 1, 0x0104, 532},
};

inline constexpr Field kFields_0187[] = {
    {"TextScalingLevels", 0, 12, 0x0024, 235},
    {"WorldSpaceOffset", 12, 4, 0x0008, 4550},
    {"IconScalingLevels", 16, 12, 0x0024, 236},
    {"WorldSpaceOffsetThreshold", 28, 4, 0x0008, 4552},
    {"Anims", 32, 12, 0x0024, 237},
    {"ScreenSpaceOffset", 44, 4, 0x0008, 4553},
    {"TextObjects", 48, 12, 0x0024, 238},
    {"DistanceThreshold", 60, 4, 0x0008, 4554},
    {"Pickups", 64, 12, 0x0024, 239},
    {"BaseScale", 76, 4, 0x0008, 4555},
    {"HideWithPickups", 80, 12, 0x0024, 240},
    {"MinScale", 92, 4, 0x0008, 4556},
    {"IconName", 96, 8, 0x0010, 0},
    {"ArrowName", 104, 8, 0x0010, 0},
    {"GoName", 112, 8, 0x0018, 0},
    {"VisSoundName", 120, 8, 0x0010, 0},
    {"VisSoundNameForHold", 128, 8, 0x0010, 0},
    {"MashToHoldIconName", 136, 8, 0x0010, 0},
    {"ScreenRangeSettings", 144, 8, 0x001C, 388},
    {"MaxScale", 152, 4, 0x0008, 4557},
    {"FixedScreenspacePosition", 156, 0, 0x002C, 3},
    {"OffscreenSafetyBounds", 160, 0, 0x002C, 3},
    {"SubIconSortIndex", 164, 4, 0x0000, 329},
    {"SubIconsStartOffset", 168, 0, 0x002C, 3},
    {"SubIconsExpandDirection", 172, 0, 0x002C, 3},
    {"SubIconScale", 176, 4, 0x0008, 4566},
    {"SubIconSize", 180, 4, 0x0008, 4567},
    {"FadeRate", 184, 4, 0x0008, 4568},
    {"AddSoldierHeightToOffset", 188, 1, 0x0014, 1190},
    {"ScaleOptions", 189, 1, 0x0204, 533},
    {"SetAtFixedScreenspacePosition", 190, 1, 0x0014, 1191},
    {"WhereVis", 191, 1, 0x0104, 534},
    {"NeverHide", 192, 1, 0x0014, 1192},
    {"UserDisplaySetting", 193, 1, 0x0104, 535},
    {"UseScreenRangeSettings", 194, 1, 0x0014, 1193},
    {"Control", 195, 1, 0x0104, 536},
    {"IsMashButton", 196, 1, 0x0014, 1194},
    {"OffscreenOptions", 197, 1, 0x0204, 537},
    {"SupportsSubIcons", 198, 1, 0x0014, 1195},
    {"StartOffsetAtMidpoint", 199, 1, 0x0014, 1196},
    {"AvailableInPhotoMode", 200, 1, 0x0014, 1197},
    {"AlwaysUpright", 201, 1, 0x0014, 1198},
};

inline constexpr Field kFields_0188[] = {
    {"IKFlags", 0, 4, 0x0000, 330},
    {"ScaleImpact", 4, 4, 0x0008, 4569},
    {"ScaleTime", 8, 4, 0x0008, 4570},
};

inline constexpr Field kFields_0189[] = {
    {"Type", 0, 1, 0x0104, 538},
    {"CameraFacingRedirectPercent", 4, 4, 0x0008, 4571},
    {"CameraFacingRedirectRandomRange", 8, 4, 0x0008, 4572},
    {"CameraFacingRedirectPercentAtMaxAngle", 12, 4, 0x0008, 4573},
    {"CameraAdjustRedirectStartAngle", 16, 4, 0x0008, 4574},
    {"CameraAdjustRedirectEndAngle", 20, 4, 0x0008, 4575},
    {"CameraAdjustRedirectStartDistance", 24, 4, 0x0008, 4576},
    {"InterestingTargetRedirectionAngle", 28, 4, 0x0008, 4577},
    {"InterestingTargetRedirectionMaximumDistance", 32, 4, 0x0008, 4578},
    {"InterestingTargetRedirectionMinimumDistance", 36, 4, 0x0008, 4579},
};

inline constexpr Field kFields_018A[] = {
    {"StatusMeterName", 0, 8, 0x0010, 0},
    {"Amount", 8, 4, 0x0008, 4580},
    {"ApplyDamageFilter", 12, 1, 0x0104, 539},
    {"ApplyDamageMode", 13, 1, 0x0104, 540},
};

inline constexpr Field kFields_018B[] = {
    {"StatusMeterDamage", 0, 8, 0x001C, 394},
    {"Decision", 8, 8, 0x001C, 1067},
};

inline constexpr Field kFields_018C[] = {
    {"BlockImpulseAwayVector", 0, 0, 0x002C, 393},
    {"GroundImpulseAwayVector", 40, 0, 0x002C, 393},
    {"AirImpulseAwayVector", 80, 0, 0x002C, 393},
    {"RagdollImpulseAwayVector", 120, 0, 0x002C, 393},
    {"BlockImpulse", 160, 0, 0x002C, 11},
    {"GroundImpulse", 174, 0, 0x002C, 11},
    {"AirImpulse", 188, 0, 0x002C, 11},
    {"RagdollImpulse", 202, 0, 0x002C, 11},
    {"StatusMeterDamage", 216, 12, 0x0024, 241},
    {"Flags", 228, 4, 0x0204, 557},
    {"StatusMeterDamageWithPickupStatusDecision", 232, 12, 0x0024, 242},
    {"MFXSwitches", 244, 0, 0x002C, 181},
    {"Effects", 256, 12, 0x0024, 244},
    {"On", 268, 2, 0x0008, 4637},
    {"Off", 270, 2, 0x0008, 4638},
    {"ImpulseCameraOverrideAngleList", 272, 12, 0x0024, 245},
    {"HitPause", 284, 2, 0x0008, 4640},
    {"BlockPause", 286, 2, 0x0008, 4641},
    {"ImpulseCameraOverrideExtraAwayValueList", 288, 12, 0x0024, 246},
    {"Damage", 300, 2, 0x0008, 4643},
    {"Stun", 302, 2, 0x0008, 4644},
    {"DamageImmunity", 304, 8, 0x001C, 1277},
    {"WeaponDamageIK", 312, 8, 0x001C, 392},
    {"PartFlags", 320, 8, 0x0204, 558},
    {"HitFlags", 328, 8, 0x0204, 559},
    {"Pickup", 336, 8, 0x001C, 1183},
    {"Context", 344, 8, 0x0010, 0},
    {"HitJoint", 352, 8, 0x0010, 0},
    {"PartFlagsBehavior", 360, 1, 0x0104, 560},
    {"Partitions", 361, 1, 0x0000, 332},
    {"IgnorePartitions_AlwaysHit", 362, 1, 0x0014, 1199},
    {"SynchID", 363, 1, 0x0000, 333},
    {"PickupSlot", 364, 1, 0x0000, 334},
    {"MFXLocationMode", 365, 1, 0x0104, 561},
    {"ForceFaceAttacker", 366, 1, 0x0014, 1200},
    {"ShouldTriggerHitEvent", 367, 1, 0x0014, 1201},
    {"AllowPlayHitReact", 368, 1, 0x0014, 1202},
    {"ForceSetLastDamageAttacker", 369, 1, 0x0014, 1203},
    {"FaceImpulseFlags", 370, 1, 0x0204, 562},
    {"IsSkillTreeCollision", 371, 1, 0x0015, 1204},
};

inline constexpr Field kFields_018D[] = {
    {"MeterName", 0, 8, 0x0010, 0},
    {"Amount", 8, 4, 0x0008, 4645},
    {"Type", 12, 1, 0x0104, 563},
    {"Params", 16, 8, 0x001C, 1310},
};

inline constexpr Field kFields_018E[] = {
    {"ModifyMeterDamage", 376, 12, 0x0024, 253},
    {"OnHitEventName", 392, 16, 0x0024, 254},
};

inline constexpr Field kFields_018F[] = {
    {"Type", 0, 1, 0x0105, 586},
    {"LifeSpan", 4, 4, 0x0008, 4710},
    {"Velocity", 8, 4, 0x0008, 4711},
    {"MaxVelocity", 12, 4, 0x0008, 4712},
    {"MinVelocity", 16, 4, 0x0008, 4713},
    {"Acceleration", 20, 4, 0x0008, 4714},
    {"Gravity", 24, 4, 0x0008, 4715},
};

inline constexpr Field kFields_0190[] = {
    {"HomingAmount", 28, 4, 0x0008, 4722},
    {"HomingDelay", 32, 4, 0x0008, 4723},
    {"HomingCancelDistanceToTarget", 36, 4, 0x0008, 4724},
    {"HomingCancelDistanceNeedsEvade", 40, 1, 0x0014, 1211},
    {"OrbitCancelAngleThreshold", 44, 4, 0x0008, 4725},
    {"OrbitCancelDistanceThreshold", 48, 4, 0x0008, 4726},
    {"FalloffOnsetTimer", 52, 4, 0x0008, 4727},
    {"FalloffGravity", 56, 4, 0x0008, 4728},
    {"TargetingPrediction", 60, 4, 0x0008, 4729},
    {"TargetingPredictionMax", 64, 4, 0x0008, 4730},
    {"TargetDistanceOffset", 68, 4, 0x0008, 4731},
};

inline constexpr Field kFields_0191[] = {
    {"Offset", 28, 0, 0x002C, 6},
};

inline constexpr Field kFields_0192[] = {
    {"LeftOrRight", 28, 1, 0x0014, 1212},
    {"UpOrDown", 29, 1, 0x0014, 1213},
    {"HorizontalLoops", 30, 1, 0x0000, 339},
    {"VerticalLoops", 31, 1, 0x0000, 340},
    {"ScaleDistance", 32, 4, 0x0008, 4747},
    {"HorizontalAmplitude", 36, 4, 0x0008, 4748},
    {"VerticalAmplitude", 40, 4, 0x0008, 4749},
};

inline constexpr Field kFields_0193[] = {
    {"Subtype", 28, 1, 0x0105, 591},
    {"CurveName", 32, 8, 0x0010, 0},
    {"StartPhase", 40, 4, 0x0008, 4756},
    {"EndPhase", 44, 4, 0x0008, 4757},
};

inline constexpr Field kFields_0194[] = {
    {"TargetAnchorPhase", 48, 4, 0x0008, 4766},
    {"StartHomingPhase", 52, 4, 0x0008, 4767},
    {"StopHomingPhase", 56, 4, 0x0008, 4768},
    {"HomingAmount", 60, 4, 0x0008, 4769},
    {"HomingCancelDistanceToTarget", 64, 4, 0x0008, 4770},
    {"StartHomingEvadeCancellablePhase", 68, 4, 0x0008, 4771},
    {"EndHomingEvadeCancellablePhase", 72, 4, 0x0008, 4772},
    {"MinRange", 76, 4, 0x0008, 4773},
    {"MaxRange", 80, 4, 0x0008, 4774},
    {"CurveRoll", 84, 4, 0x0008, 4775},
    {"HorizontalScaleConstant", 88, 4, 0x0008, 4776},
    {"HorizontalScaleWithDistance", 92, 4, 0x0008, 4777},
    {"HorizontalScaleMin", 96, 4, 0x0008, 4778},
    {"HorizontalScaleMax", 100, 4, 0x0008, 4779},
    {"VerticalScaleConstant", 104, 4, 0x0008, 4780},
    {"VerticalScaleWithDistance", 108, 4, 0x0008, 4781},
    {"VerticalScaleMin", 112, 4, 0x0008, 4782},
    {"VerticalScaleMax", 116, 4, 0x0008, 4783},
    {"TargetingPredictionAmount", 120, 4, 0x0008, 4784},
    {"PredictionCancelDistanceToTarget", 124, 4, 0x0008, 4785},
    {"PredictionCancelPhase", 128, 4, 0x0008, 4786},
    {"HomingCancelDistanceNeedsEvade", 132, 1, 0x0014, 1214},
    {"HorizontalMirror", 133, 1, 0x0014, 1215},
    {"VerticalMirror", 134, 1, 0x0014, 1216},
    {"PredictionCancelNeedsEvade", 135, 1, 0x0014, 1217},
};

inline constexpr Field kFields_0195[] = {
    {"PathFlags", 48, 1, 0x0204, 596},
    {"RotateX", 52, 4, 0x0008, 4795},
    {"RotateY", 56, 4, 0x0008, 4796},
    {"RotateZ", 60, 4, 0x0008, 4797},
    {"ScaleX", 64, 4, 0x0008, 4798},
    {"ScaleMinX", 68, 4, 0x0008, 4799},
    {"ScaleMaxX", 72, 4, 0x0008, 4800},
    {"ScaleY", 76, 4, 0x0008, 4801},
    {"ScaleMinY", 80, 4, 0x0008, 4802},
    {"ScaleMaxY", 84, 4, 0x0008, 4803},
    {"ScaleZ", 88, 4, 0x0008, 4804},
    {"ScaleMinZ", 92, 4, 0x0008, 4805},
    {"ScaleMaxZ", 96, 4, 0x0008, 4806},
};

inline constexpr Field kFields_0196[] = {
    {"When", 0, 8, 0x0030, 65535},
    {"Priority", 8, 4, 0x0008, 4807},
};

inline constexpr Field kFields_0197[] = {
    {"ScaleDamage", 16, 4, 0x0008, 4809},
    {"ScaleStatusMeterDamage", 20, 4, 0x0008, 4810},
    {"Payload", 24, 8, 0x001C, 257},
    {"PayloadForSheetContact", 32, 8, 0x001C, 257},
    {"CollisionFX", 40, 8, 0x001C, 272},
};

inline constexpr Field kFields_0198[] = {
    {"HalfConeWidthInner", 16, 4, 0x0008, 4812},
    {"HalfConeWidthOuter", 20, 4, 0x0008, 4813},
    {"Falloff", 24, 1, 0x0104, 597},
};

inline constexpr Field kFields_0199[] = {
    {"ImpactOverrides", 0, 12, 0x0024, 255},
    {"AtDistance", 12, 4, 0x0008, 4814},
    {"TargetingOverrides", 16, 12, 0x0024, 256},
};

inline constexpr Field kFields_019A[] = {
    {"StuckTime", 0, 4, 0x0008, 4815},
    {"FadeTime", 4, 4, 0x0008, 4816},
    {"DeflectionFactor", 8, 4, 0x0008, 4817},
    {"MaxDeflections", 12, 1, 0x0000, 341},
    {"RigidBodyJointName", 16, 8, 0x0010, 118},
    {"ArrowGOName", 24, 8, 0x0018, 0},
    {"TrailGOName", 32, 8, 0x0018, 0},
    {"CollisionFX", 40, 8, 0x001C, 272},
    {"ArrowTint", 48, 0, 0x002C, 0},
    {"BlockSound", 56, 8, 0x0010, 0},
    {"HitSoundList", 64, 12, 0x0024, 257},
    {"SyncedSound", 80, 8, 0x0018, 0},
    {"SyncedSoundEmitter", 88, 8, 0x0018, 0},
    {"Collision", 96, 0, 0x002C, 396},
    {"EnvCollisionStartDelay", 472, 4, 0x0008, 4886},
    {"CreatureCollisionStartDelay", 476, 4, 0x0008, 4887},
    {"MaxVisualToLogicalBlendTime", 480, 4, 0x0008, 4888},
    {"ArrowOverrides", 488, 12, 0x0024, 264},
    {"HeroUseZeroRadiusEnvCollision", 500, 1, 0x0014, 1224},
    {"PierceEnvIfNoPrecisionTarget", 501, 1, 0x0014, 1225},
    {"ForceEnvPrecisionTarget", 502, 1, 0x0014, 1226},
    {"ForceEnvPayloadAfterPiercingCreatures", 503, 1, 0x0014, 1227},
    {"IgnoreStuckTimeForSheetPayload", 504, 1, 0x0014, 1228},
    {"CollisionRadius", 508, 4, 0x0008, 4889},
    {"NumCollisionRays", 512, 1, 0x0000, 346},
    {"Flags", 516, 4, 0x0204, 620},
    {"Payload", 520, 8, 0x001C, 257},
    {"PayloadForSheetContact", 528, 8, 0x001C, 257},
    {"AttackableHealth", 536, 4, 0x0008, 4890},
    {"AttackableOnsetTimer", 540, 4, 0x0008, 4891},
    {"AttackablePayload", 544, 8, 0x001C, 257},
    {"AttackableCollisionFX", 552, 8, 0x001C, 272},
    {"AttackableKillFX", 560, 8, 0x001C, 272},
    {"AttackableHitModifiers", 568, 12, 0x0024, 265},
    {"InvalidThrowableResponse", 584, 8, 0x0010, 119},
    {"OverrideExposedSideWithAttackerPos", 592, 1, 0x0014, 1229},
    {"StickPointJoint", 600, 8, 0x0010, 0},
    {"MovementList", 608, 12, 0x0024, 266},
    {"CameraTargets", 624, 12, 0x0024, 267},
    {"SoundEnableProximityTrigger", 636, 1, 0x0014, 1230},
    {"SoundAction", 640, 8, 0x001C, 177},
    {"SoundWindowAction", 648, 8, 0x001C, 972},
    {"Orientation", 656, 1, 0x0104, 621},
    {"AccessibilityHighlightCategory", 657, 1, 0x0104, 622},
    {"ScaleX", 660, 4, 0x0008, 4892},
    {"ScaleY", 664, 4, 0x0008, 4893},
    {"ScaleZ", 668, 4, 0x0008, 4894},
    {"RotateX", 672, 4, 0x0008, 4895},
    {"RotateY", 676, 4, 0x0008, 4896},
    {"RotateZ", 680, 4, 0x0008, 4897},
    {"RotationSpeedX", 684, 4, 0x0008, 4898},
    {"RotationSpeedY", 688, 4, 0x0008, 4899},
    {"RotationSpeedZ", 692, 4, 0x0008, 4900},
};

inline constexpr Field kFields_019B[] = {
    {"EmitList", 0, 12, 0x0024, 268},
    {"AmountGameSlowDown", 12, 4, 0x0008, 4901},
    {"AimJoint", 16, 8, 0x0018, 0},
    {"ReticleName", 24, 8, 0x0010, 0},
    {"ConsumeToken", 32, 8, 0x0010, 0},
    {"ReticleControl", 40, 8, 0x001C, 1497},
    {"AimJointDir", 48, 0, 0x002C, 6},
    {"Flags", 54, 1, 0x0204, 623},
    {"TargetZeroJoint", 55, 1, 0x0014, 1231},
    {"AccuracyRadius", 56, 4, 0x0008, 4905},
    {"MuzzleRadius", 60, 4, 0x0008, 4906},
    {"HighTrajectoryDistance", 64, 4, 0x0008, 4907},
    {"Heap", 68, 4, 0x0000, 347},
    {"ForceTargetJointIndex", 72, 4, 0x0000, 348},
    {"TargetOffsetForward", 76, 4, 0x0008, 4908},
    {"TargetOffsetUp", 80, 4, 0x0008, 4909},
    {"TargetOffsetLeft", 84, 4, 0x0008, 4910},
    {"TargetOffsetFlags", 88, 1, 0x0204, 624},
    {"ConsiderTargetVelocity", 89, 1, 0x0014, 1232},
};

inline constexpr Field kFields_019C[] = {
    {"Collision", 0, 0, 0x002C, 396},
    {"PayloadForCreatureContact", 376, 8, 0x001C, 257},
    {"PayloadForSheetContact", 384, 8, 0x001C, 257},
    {"BeamFX", 392, 8, 0x001C, 272},
    {"StartBeamFX", 400, 8, 0x001C, 272},
    {"FXOnCollisionName", 408, 8, 0x0010, 0},
    {"SoundEventName", 416, 8, 0x0018, 0},
    {"SoundEmitterName", 424, 8, 0x0018, 0},
    {"Radius", 432, 4, 0x0008, 4975},
    {"Length", 436, 4, 0x0008, 4976},
    {"PayloadCoolDown", 440, 4, 0x0008, 4977},
    {"Flags", 444, 1, 0x0204, 647},
    {"ShouldGoThroughCreatures", 445, 1, 0x0014, 1239},
    {"ShouldGoThroughShield", 446, 1, 0x0014, 1240},
    {"ShouldIgnoreCompanions", 447, 1, 0x0014, 1241},
};

inline constexpr Field kFields_019D[] = {
    {"HitResponseName", 0, 8, 0x0010, 0},
    {"StartNewMode", 8, 8, 0x001C, 423},
    {"PhysicsDeflect", 16, 8, 0x001C, 421},
    {"OnHitAction", 24, 1, 0x0104, 648},
    {"MaxNumberBounces", 25, 1, 0x0000, 353},
};

inline constexpr Field kFields_019E[] = {
    {"JointName", 0, 8, 0x0010, 0},
    {"JointOffset", 8, 0, 0x002C, 6},
    {"JointRotation", 14, 0, 0x002C, 6},
};

inline constexpr Field kFields_019F[] = {
    {"ReticleSubName", 0, 8, 0x0010, 0},
    {"DebugReticleText", 8, 8, 0x0018, 0},
    {"DebugOffsetX", 16, 4, 0x0008, 4984},
    {"DebugOffsetY", 20, 4, 0x0008, 4985},
};

inline constexpr Field kFields_01A0[] = {
    {"TargetEmbedJoints", 0, 12, 0x0024, 275},
    {"OverrideDistance", 12, 4, 0x0008, 4986},
    {"ReticleTargetGroupName", 16, 8, 0x0018, 0},
    {"TargetSubReticle", 24, 8, 0x001C, 415},
    {"MinLookAngle", 32, 4, 0x0008, 4987},
    {"MaxLookAngle", 36, 4, 0x0008, 4988},
};

inline constexpr Field kFields_01A1[] = {
    {"WeaponType", 0, 4, 0x0000, 354},
    {"Original", 8, 8, 0x0010, 0},
    {"Override", 16, 8, 0x0010, 0},
    {"MinHitAngle", 24, 4, 0x0008, 4989},
    {"MaxHitAngle", 28, 4, 0x0008, 4990},
};

inline constexpr Field kFields_01A2[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"ResponseOverrides", 8, 12, 0x0024, 276},
    {"TargetGroups", 24, 12, 0x0024, 277},
    {"Flags", 36, 1, 0x0204, 649},
    {"IgnoreAll", 37, 1, 0x0014, 1242},
};

inline constexpr Field kFields_01A3[] = {
    {"HitType", 0, 8, 0x0010, 0},
    {"OverrideHitType", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_01A4[] = {
    {"HitReaction", 0, 8, 0x0010, 0},
    {"HitResponse", 8, 12, 0x0024, 278},
};

inline constexpr Field kFields_01A5[] = {
    {"SpeedType", 0, 1, 0x0104, 650},
    {"DeflectMinSpeed", 4, 4, 0x0008, 4991},
    {"DeflectMaxSpeed", 8, 4, 0x0008, 4992},
    {"DeflectMaxAngle", 12, 4, 0x0008, 4993},
    {"DeflectConeVerticalBias", 16, 4, 0x0008, 4994},
    {"DeflectConeHorizontalBias", 20, 4, 0x0008, 4995},
    {"StartNewMode", 24, 8, 0x001C, 423},
};

inline constexpr Field kFields_01A6[] = {
    {"Attachment", 0, 8, 0x0010, 120},
    {"AttachName", 8, 8, 0x0010, 0},
    {"SurfaceMaterial", 16, 8, 0x0010, 0},
    {"Side", 24, 4, 0x0000, 355},
    {"Status", 28, 2, 0x0204, 651},
    {"EmbeddedInMe", 30, 1, 0x0104, 652},
    {"StuckState", 31, 1, 0x0104, 653},
    {"CanThrowAtTarget", 32, 1, 0x0014, 1243},
};

inline constexpr Field kFields_01A7[] = {
    {"Speed", 0, 0, 0x002C, 229},
    {"ThrowName", 40, 8, 0x0018, 0},
    {"Collision", 48, 8, 0x001C, 396},
    {"OnHitAction", 56, 8, 0x0010, 121},
    {"OnHitWorldAction", 64, 8, 0x0010, 122},
    {"OnStartReturnModeOverride", 72, 8, 0x0010, 0},
    {"EmbedJointName", 80, 8, 0x0018, 123},
    {"HitSurfaceJointName", 88, 8, 0x0018, 124},
    {"ModelRootJointName", 96, 8, 0x0018, 125},
    {"SeekTargetMode", 104, 8, 0x001C, 435},
    {"OverrideDefaultReturnMode", 112, 8, 0x001C, 423},
    {"ThrowEffect", 120, 8, 0x001C, 272},
    {"Sound", 128, 8, 0x0018, 0},
    {"Emitter", 136, 8, 0x0018, 0},
    {"SyncSound", 144, 8, 0x0018, 0},
    {"SyncEmitter", 152, 8, 0x0018, 0},
    {"NonReticleThrowFromJointName", 160, 8, 0x0018, 126},
    {"NonReticleThrowToJointName", 168, 8, 0x0018, 127},
    {"RotationAxis", 176, 0, 0x002C, 6},
    {"RotateSurfaceNormalEmbed", 182, 0, 0x002C, 6},
    {"FollowupEmbedTargetOffset", 188, 0, 0x002C, 6},
    {"CatchLocalOffset", 194, 0, 0x002C, 6},
    {"ThrowRelease", 200, 4, 0x0008, 5014},
    {"MaxSpeed", 204, 4, 0x0008, 5015},
    {"TimeDilation", 208, 4, 0x0008, 5016},
    {"StartSpeedOverride", 212, 4, 0x0008, 5017},
    {"ReleaseAcceleration", 216, 4, 0x0008, 5018},
    {"RevolutionsPerMeter", 220, 4, 0x0008, 5019},
    {"OutwardDistance", 224, 4, 0x0008, 5020},
    {"EaseIn", 228, 4, 0x0008, 5021},
    {"AxeSpinAxis", 232, 4, 0x0008, 5022},
    {"OffsetRotationCenter", 236, 4, 0x0008, 5023},
    {"ThrowableTargetsWeight", 240, 4, 0x0008, 5024},
    {"ThrowableTargetsMaxSeekAngleInitial", 244, 4, 0x0008, 5025},
    {"ThrowableTargetsMaxSeekAngleInFlight", 248, 4, 0x0008, 5026},
    {"MinFreeThrowVerticalAngle", 252, 4, 0x0008, 5027},
    {"CollisionDeadTime", 256, 4, 0x0008, 5028},
    {"EmbedMask", 260, 4, 0x0000, 356},
    {"AlignToSurfaceNormalAmount", 264, 4, 0x0008, 5029},
    {"AlignToSurfaceMaxAngle", 268, 4, 0x0008, 5030},
    {"AlignToSurfaceFlipThreshold", 272, 4, 0x0008, 5031},
    {"GrindTime", 276, 4, 0x0008, 5032},
    {"BaseOffsetAngle", 280, 4, 0x0008, 5033},
    {"HitRotationRandomAngle", 284, 4, 0x0008, 5034},
    {"EmbedMinDistance", 288, 4, 0x0008, 5035},
    {"EmbedMaxDistance", 292, 4, 0x0008, 5036},
    {"BounceSlowSpeedFactor", 296, 4, 0x0008, 5037},
    {"MinAngleToGlance", 300, 4, 0x0008, 5038},
    {"GlancingRotationAngle", 304, 4, 0x0008, 5039},
    {"GlancingDistanceExtend", 308, 4, 0x0008, 5040},
    {"GlancingBounceAmount", 312, 4, 0x0008, 5041},
    {"MaxTargets", 316, 4, 0x0000, 357},
    {"AutoTargetMaxAngle", 320, 4, 0x0008, 5042},
    {"AutoTargetMaxDistance", 324, 4, 0x0008, 5043},
    {"SecondaryTargetMaxDistance", 328, 4, 0x0008, 5044},
    {"MultiHitMaxLength", 332, 4, 0x0008, 5045},
    {"BounceAmount", 336, 4, 0x0008, 5046},
    {"NonReticleThrowHorizontalMaxTrackingAngle", 340, 4, 0x0008, 5047},
    {"NonReticleThrowVerticalMaxTrackingAngle", 344, 4, 0x0008, 5048},
    {"ThrowStatePickup", 348, 2, 0x0000, 358},
    {"Type", 350, 1, 0x0105, 657},
    {"FollowPath", 351, 1, 0x0014, 1244},
    {"ForceUseCurrentTarget", 352, 1, 0x0014, 1245},
    {"SeekTarget", 353, 1, 0x0014, 1246},
    {"ThrowFlags", 354, 1, 0x0204, 658},
    {"AlignToSurface", 355, 1, 0x0014, 1247},
    {"AlignToSurfaceNormal", 356, 1, 0x0014, 1248},
    {"IsRotateToEmbed", 357, 1, 0x0014, 1249},
    {"AllowAcquireTarget", 358, 1, 0x0014, 1250},
    {"TargetType", 359, 1, 0x0104, 659},
    {"AutoTargetVertical", 360, 1, 0x0014, 1251},
    {"AutoTargetCurve", 361, 1, 0x0014, 1252},
    {"ThrowableSeamlessPhysics", 362, 1, 0x0014, 1253},
    {"UpdateOnEmbed", 363, 1, 0x0014, 1254},
    {"KeepPickUpOnUntilNextThrow", 364, 1, 0x0014, 1255},
    {"FauxEmbedded", 365, 1, 0x0014, 1256},
};

inline constexpr Field kFields_01A8[] = {
    {"OutwardVertical", 368, 4, 0x0008, 5102},
    {"RotateArc", 372, 4, 0x0008, 5103},
    {"LobTossCurveOut", 376, 4, 0x0008, 5104},
    {"LobTossCurveTarget", 380, 4, 0x0008, 5105},
    {"LobTossCurveTop", 384, 4, 0x0008, 5106},
    {"LobTossCurveToHand", 388, 4, 0x0008, 5107},
};

inline constexpr Field kFields_01A9[] = {
    {"ConcussionOffset", 368, 4, 0x0008, 5161},
    {"EmbedDelayFromLastConcussion", 372, 4, 0x0008, 5162},
    {"DrillConcussionList", 376, 12, 0x0024, 282},
    {"TargetJointName", 392, 8, 0x0010, 0},
    {"ForceMoveOnWeapon", 400, 8, 0x001C, 1283},
    {"AllowPiercingOnKill", 408, 1, 0x0014, 1283},
    {"PiercingThrowSpeedModifier", 412, 4, 0x0008, 5163},
    {"PiercingDelayFromKill", 416, 4, 0x0008, 5164},
    {"CollisionShrinkRatio", 420, 4, 0x0008, 5165},
    {"ResistanceFrequency", 424, 4, 0x0008, 5166},
    {"ResistanceMaxAngle", 428, 4, 0x0008, 5167},
};

inline constexpr Field kFields_01AA[] = {
    {"BaseOffset", 368, 0, 0x002C, 6},
    {"AngularVelocity", 374, 0, 0x002C, 6},
    {"ScaleGravity", 380, 4, 0x0008, 5227},
    {"CollisionBehindDistance", 384, 4, 0x0008, 5228},
    {"CollisionAheadDistance", 388, 4, 0x0008, 5229},
    {"RotationTweenTime", 392, 4, 0x0008, 5230},
};

inline constexpr Field kFields_01AB[] = {
    {"SpinTime", 368, 4, 0x0008, 5284},
    {"StartRotationSpeed", 372, 4, 0x0008, 5285},
    {"TargetRevolutionsPerSecond", 376, 4, 0x0008, 5286},
    {"RotationalAcceleration", 380, 4, 0x0008, 5287},
    {"StickToLastCollision", 384, 1, 0x0014, 1310},
    {"GrindJointName", 392, 8, 0x0010, 0},
    {"StartNewMode", 400, 8, 0x001C, 423},
};

inline constexpr Field kFields_01AC[] = {
    {"GrindPickup", 368, 2, 0x0000, 374},
    {"RotationalAccelerationAcceleration", 372, 4, 0x0008, 5341},
    {"RotationStartSpeed", 376, 4, 0x0008, 5342},
    {"RotationalAcceleration", 380, 4, 0x0008, 5343},
    {"GrindRotationalAcceleration", 384, 4, 0x0008, 5344},
    {"MaxRotateSpeedPerSecond", 388, 4, 0x0008, 5345},
    {"HeightOffGround", 392, 4, 0x0008, 5346},
    {"ThrowDownAngle", 396, 4, 0x0008, 5347},
    {"PerturbAmount", 400, 4, 0x0008, 5348},
    {"PerturbFrequencyScale", 404, 4, 0x0008, 5349},
    {"PerturbOffsetScale", 408, 4, 0x0008, 5350},
    {"ScaleFlairThrowDownArc", 412, 4, 0x0008, 5351},
    {"DropPause", 416, 4, 0x0008, 5352},
    {"StickDistance", 420, 4, 0x0008, 5353},
    {"UseReticle", 424, 1, 0x0014, 1324},
    {"RespondToCreatureCollision", 425, 1, 0x0014, 1325},
    {"StickToTarget", 426, 1, 0x0014, 1326},
};

inline constexpr Field kFields_01AD[] = {
    {"DeflectSmashPickup", 368, 2, 0x0000, 378},
    {"ScaleGravity", 372, 4, 0x0008, 5407},
    {"MaxDeflectSpinAxisOnHit", 376, 4, 0x0008, 5408},
    {"DeflectMinRotateSpeed", 380, 4, 0x0008, 5409},
    {"DeflectionMinSpeed", 384, 4, 0x0008, 5410},
    {"DeflectBounciness", 388, 4, 0x0008, 5411},
    {"DeflectRotateAmount", 392, 4, 0x0008, 5412},
    {"DeflectRandomRotationXZ", 396, 4, 0x0008, 5413},
    {"DeflectUpAngle", 400, 4, 0x0008, 5414},
    {"DeflectStopRadius", 404, 4, 0x0008, 5415},
    {"DeflectStartSlowDownPercent", 408, 4, 0x0008, 5416},
    {"DeflectStopPercent", 412, 4, 0x0008, 5417},
    {"DeflectTimeToStop", 416, 4, 0x0008, 5418},
    {"DeflectPostStopAcceleratePerSecond", 420, 4, 0x0008, 5419},
    {"DeflectMaxSpeed", 424, 4, 0x0008, 5420},
    {"DeflectTimeDilationStart", 428, 4, 0x0008, 5421},
    {"DeflectMinPercentSlowTime", 432, 4, 0x0008, 5422},
    {"DeflectTimeSpeedupFactor", 436, 4, 0x0008, 5423},
    {"SpeedUpTimeFactor", 440, 4, 0x0008, 5424},
    {"MaxRotateSpeedPerSecond", 444, 4, 0x0008, 5425},
    {"RespondToCreatureCollision", 448, 1, 0x0014, 1340},
    {"UseSurfaceReflection", 449, 1, 0x0014, 1341},
    {"OrientTowardsTrajectory", 450, 1, 0x0014, 1342},
};

inline constexpr Field kFields_01AE[] = {
    {"MaxRange", 368, 4, 0x0008, 5479},
    {"ScaleGravity", 372, 4, 0x0008, 5480},
    {"ScaleDamping", 376, 4, 0x0008, 5481},
    {"WeaponPositionTweenTime", 380, 4, 0x0008, 5482},
    {"CurvatureBeforeTarget", 384, 4, 0x0008, 5483},
};

inline constexpr Field kFields_01AF[] = {
    {"MaxRange", 368, 4, 0x0008, 5537},
    {"ScaleGravity", 372, 4, 0x0008, 5538},
    {"TautRotateSpeed", 376, 4, 0x0008, 5539},
    {"ScaleGravityOnTaut", 380, 4, 0x0008, 5540},
    {"MinThrowOutSpeed", 384, 4, 0x0008, 5541},
    {"MaxThrowOutSpeed", 388, 4, 0x0008, 5542},
    {"MinThrowYSpeed", 392, 4, 0x0008, 5543},
    {"MaxThrowYSpeed", 396, 4, 0x0008, 5544},
    {"MaxHoldTime", 400, 4, 0x0008, 5545},
    {"PercentSlackRelease", 404, 4, 0x0008, 5546},
    {"SlackInitialVelocity", 408, 0, 0x002C, 6},
    {"UseReticle", 414, 1, 0x0014, 1369},
    {"KeepTarget", 415, 1, 0x0014, 1370},
    {"WeaponPositionTweenTime", 416, 4, 0x0008, 5550},
};

inline constexpr Field kFields_01B0[] = {
    {"CurveStartDepthBias", 368, 4, 0x0008, 5604},
    {"CurveStartHorizontalBias", 372, 4, 0x0008, 5605},
    {"CurveStartVerticalBias", 376, 4, 0x0008, 5606},
    {"CurveEndDepthBias", 380, 4, 0x0008, 5607},
    {"CurveEndHorizontalBias", 384, 4, 0x0008, 5608},
    {"CurveEndVerticalBias", 388, 4, 0x0008, 5609},
    {"MaxReturnTime", 392, 4, 0x0008, 5610},
    {"ForceCatchPos", 396, 4, 0x0008, 5611},
    {"YankRotation", 400, 0, 0x002C, 6},
    {"RotationCycles", 406, 1, 0x0000, 388},
    {"RotationStartPercent", 408, 4, 0x0008, 5615},
    {"EaseOut", 412, 4, 0x0008, 5616},
    {"OnWeaponUnembedHook", 416, 8, 0x0018, 0},
};

inline constexpr Field kFields_01B1[] = {
    {"LaunchMode", 368, 1, 0x0104, 720},
    {"GoToTargetInStraightLineLaunchMode", 369, 1, 0x0014, 1397},
    {"LaunchSpeed", 372, 4, 0x0008, 5670},
    {"LaunchAngleVertical", 376, 4, 0x0008, 5671},
    {"LaunchAngleHorizontal", 380, 4, 0x0008, 5672},
    {"AccuracyRadius", 384, 4, 0x0008, 5673},
};

inline constexpr Field kFields_01B3[] = {
    {"MinLoopSize", 368, 4, 0x0008, 5780},
    {"DistanceToActAsSelfLoop", 372, 4, 0x0008, 5781},
    {"MinLoopMultiplier", 376, 4, 0x0008, 5782},
    {"MaxLoopMultiplier", 380, 4, 0x0008, 5783},
    {"SeekDelay", 384, 4, 0x0008, 5784},
    {"MinCurveOffsetLength", 388, 4, 0x0008, 5785},
    {"MaxCurveOffsetLength", 392, 4, 0x0008, 5786},
    {"CurveOffsetLerpDistance", 396, 4, 0x0008, 5787},
    {"SelfLoopSlowdown", 400, 4, 0x0008, 5788},
};

inline constexpr Field kFields_01B4[] = {
    {"OnWeaponUnembedHook", 368, 8, 0x0018, 0},
    {"WiggleSound", 376, 8, 0x0018, 0},
    {"WiggleEmitter", 384, 8, 0x0018, 0},
    {"WeaponFlag", 392, 8, 0x0010, 0},
    {"MaxReturnLength", 400, 4, 0x0008, 5842},
    {"WarpSkyOffset", 404, 4, 0x0008, 5843},
    {"StartSpeed", 408, 4, 0x0008, 5844},
    {"Acceleration", 412, 4, 0x0008, 5845},
    {"RotationAcceleration", 416, 4, 0x0008, 5846},
    {"MaxReturnTime", 420, 4, 0x0008, 5847},
    {"MinReturnSpeed", 424, 4, 0x0008, 5848},
    {"CurveAmount", 428, 4, 0x0008, 5849},
    {"MinTangentSize", 432, 4, 0x0008, 5850},
    {"MinTangentDistance", 436, 4, 0x0008, 5851},
    {"MaxTangentSize", 440, 4, 0x0008, 5852},
    {"MaxTangentDistance", 444, 4, 0x0008, 5853},
    {"ForceCatchPos", 448, 4, 0x0008, 5854},
    {"ReturnDelayTime", 452, 4, 0x0008, 5855},
    {"ShakePercent", 456, 4, 0x0008, 5856},
    {"ShakeAngle", 460, 4, 0x0008, 5857},
    {"RipOutDistance", 464, 4, 0x0008, 5858},
    {"WeaponMinDistance", 468, 4, 0x0008, 5859},
    {"WeaponMaxDistance", 472, 4, 0x0008, 5860},
    {"CallClosestWeapon", 476, 1, 0x0014, 1437},
    {"IncludeOwnWeapon", 477, 1, 0x0014, 1438},
    {"IncludeWeaponWithoutOwner", 478, 1, 0x0014, 1439},
    {"IncludeWeaponWithOtherOwner", 479, 1, 0x0014, 1440},
};

inline constexpr Field kFields_01B6[] = {
    {"ThrowDownAngle", 368, 4, 0x0008, 5967},
    {"WeaponPositionTweenTime", 372, 4, 0x0008, 5968},
    {"Gravity", 376, 4, 0x0008, 5969},
    {"ThrowDownAutoTarget", 380, 1, 0x0014, 1467},
    {"HitConcussionList", 384, 12, 0x0024, 296},
    {"pointer_3", 400, 8, 0x001C, 268},
};

inline constexpr Field kFields_01B7[] = {
    {"WhirlWindTime", 368, 4, 0x0008, 6023},
    {"WhirlWindRadius", 372, 4, 0x0008, 6024},
    {"WhirlWindSpeedScale", 376, 4, 0x0008, 6025},
    {"WhirlWindFerocity", 380, 4, 0x0008, 6026},
    {"CurveOut", 384, 4, 0x0008, 6027},
    {"CurveTarget", 388, 4, 0x0008, 6028},
};

inline constexpr Field kFields_01B8[] = {
    {"Speed", 0, 0, 0x002C, 229},
    {"EmbedJointName", 40, 8, 0x0018, 240},
    {"HitSurfaceJointName", 48, 8, 0x0018, 241},
    {"ModelRootJointName", 56, 8, 0x0018, 242},
    {"Sound", 64, 8, 0x0018, 0},
    {"Emitter", 72, 8, 0x0018, 0},
    {"ThrowRelease", 80, 4, 0x0008, 6035},
    {"ReleaseAcceleration", 84, 4, 0x0008, 6036},
    {"RevolutionsPerMeter", 88, 4, 0x0008, 6037},
    {"OutwardDistance", 92, 4, 0x0008, 6038},
    {"OutwardVertical", 96, 4, 0x0008, 6039},
    {"EaseIn", 100, 4, 0x0008, 6040},
    {"MaxRange", 104, 4, 0x0008, 6041},
    {"ThrowDownAngle", 108, 4, 0x0008, 6042},
    {"LobTossCurveOut", 112, 4, 0x0008, 6043},
    {"LobTossCurveTarget", 116, 4, 0x0008, 6044},
    {"LobTossCurveTop", 120, 4, 0x0008, 6045},
    {"LobTossCurveToHand", 124, 4, 0x0008, 6046},
    {"WhirlWindTime", 128, 4, 0x0008, 6047},
    {"WhirlWindRadius", 132, 4, 0x0008, 6048},
    {"WhirlWindSpeedScale", 136, 4, 0x0008, 6049},
    {"WhirlWindFerocity", 140, 4, 0x0008, 6050},
    {"ScaleGravity", 144, 4, 0x0008, 6051},
    {"OffsetRotationCenter", 148, 4, 0x0008, 6052},
    {"BaseOffsetAngle", 152, 4, 0x0008, 6053},
    {"HitRotationRandomAngle", 156, 4, 0x0008, 6054},
    {"GrindTime", 160, 4, 0x0008, 6055},
    {"BounceSlowSpeedFactor", 164, 4, 0x0008, 6056},
    {"GlancingMaxAngle", 168, 4, 0x0008, 6057},
    {"GlancingRotationAngle", 172, 4, 0x0008, 6058},
    {"GlancingDistanceExtend", 176, 4, 0x0008, 6059},
    {"GlancingBounceAmount", 180, 4, 0x0008, 6060},
    {"MaxTargets", 184, 4, 0x0000, 410},
    {"FirstTargetMaxAngle", 188, 4, 0x0008, 6061},
    {"FirstTargetMaxDistance", 192, 4, 0x0008, 6062},
    {"SecondaryTargetMaxDistance", 196, 4, 0x0008, 6063},
    {"MultiHitMaxLength", 200, 4, 0x0008, 6064},
    {"ForceUseCurrentTarget", 204, 1, 0x0014, 1481},
    {"ThrowDownAutoTarget", 205, 1, 0x0014, 1482},
    {"UseGravityCurve", 206, 1, 0x0014, 1483},
    {"OnHitAction", 207, 1, 0x0104, 760},
    {"AutoTargetVertical", 208, 1, 0x0014, 1484},
    {"AutoTargetCurve", 209, 1, 0x0014, 1485},
};

inline constexpr Field kFields_01B9[] = {
    {"TautCurveAmount", 0, 4, 0x0008, 6065},
    {"TautNoise", 4, 4, 0x0008, 6066},
    {"TautStartForwardBias", 8, 4, 0x0008, 6067},
    {"TautStartSideBias", 12, 4, 0x0008, 6068},
    {"TautEndForwardBias", 16, 4, 0x0008, 6069},
    {"TautEndSideBias", 20, 4, 0x0008, 6070},
    {"BlendAmount", 24, 4, 0x0008, 6071},
    {"BlendStartPercent", 28, 4, 0x0008, 6072},
    {"BlendEndPercent", 32, 4, 0x0008, 6073},
    {"BlendTime", 36, 4, 0x0008, 6074},
    {"BlendTransitionLength", 40, 4, 0x0008, 6075},
    {"ForceTaut", 44, 1, 0x0014, 1486},
    {"AnimBlendType", 45, 1, 0x0104, 761},
    {"EnableIKFixup", 46, 1, 0x0014, 1487},
};

inline constexpr Field kFields_01BA[] = {
    {"SlackLength", 0, 4, 0x0008, 6076},
    {"SlackRecoilSpeed", 4, 4, 0x0008, 6077},
    {"MinSlackToRecoil", 8, 4, 0x0008, 6078},
    {"MinLinkDistance", 12, 4, 0x0008, 6079},
    {"MaxLinkDistance", 16, 4, 0x0008, 6080},
    {"ShapeSettings", 24, 8, 0x001C, 441},
};

inline constexpr Field kFields_01BB[] = {
    {"HitType", 0, 1, 0x0104, 762},
    {"EmbedInCreatures", 1, 1, 0x0000, 411},
    {"DeflectData", 8, 8, 0x001C, 421},
    {"RecoilSpeed", 16, 4, 0x0008, 6081},
    {"GrindLength", 20, 4, 0x0008, 6082},
    {"MinEmbedDepth", 24, 4, 0x0008, 6083},
    {"MaxEmbedDepth", 28, 4, 0x0008, 6084},
    {"MinRotateOnEmbed", 32, 4, 0x0008, 6085},
    {"MaxRotateOnEmbed", 36, 4, 0x0008, 6086},
};

inline constexpr Field kFields_01BC[] = {
    {"Type", 0, 1, 0x0105, 763},
    {"Comparison", 1, 1, 0x0104, 764},
    {"NextOperation", 2, 1, 0x0104, 765},
};

inline constexpr Field kFields_01BD[] = {
    {"State", 3, 1, 0x0104, 769},
    {"QuestName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_01BE[] = {
    {"Wallet", 8, 8, 0x0010, 0},
    {"Resource", 16, 8, 0x0010, 0},
    {"Amount", 24, 4, 0x0000, 412},
};

inline constexpr Field kFields_01BF[] = {
    {"Level", 3, 1, 0x0000, 413},
    {"Wallet", 8, 8, 0x0010, 0},
    {"Equipment", 16, 8, 0x0010, 0},
    {"ExtraEquipment", 24, 8, 0x0010, 0},
    {"OverrideLevelTrait", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_01C0[] = {
    {"Label", 8, 8, 0x0010, 0},
    {"NumberValue", 16, 4, 0x0008, 6087},
    {"StringValue", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_01C1[] = {
    {"Loaded", 3, 1, 0x0014, 1488},
    {"WadName", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_01C2[] = {
    {"SkillTree", 8, 8, 0x0018, 0},
    {"SkillTreeNode", 16, 8, 0x0018, 0},
};

inline constexpr Field kFields_01C5[] = {
    {"Type", 0, 1, 0x0105, 791},
};

inline constexpr Field kFields_01C6[] = {
    {"SoftSave", 1, 1, 0x0014, 1489},
    {"AdditionalLams", 8, 12, 0x0024, 299},
    {"OverrideLams", 20, 4, 0x0000, 415},
    {"AwardName", 24, 8, 0x0010, 0},
    {"DistributorName", 32, 8, 0x0010, 0},
    {"WalletName", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_01C9[] = {
    {"HeaderLams", 4, 4, 0x0000, 420},
    {"BodyLams", 8, 4, 0x0000, 421},
    {"CategoryLams", 12, 4, 0x0000, 422},
    {"DisplayTime", 16, 4, 0x0008, 6088},
    {"CountOwner", 20, 1, 0x0104, 796},
    {"MessageType", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_01CA[] = {
    {"TrophyID", 4, 4, 0x0000, 423},
};

inline constexpr Field kFields_01CB[] = {
    {"Label", 8, 8, 0x0010, 0},
    {"StringValue", 16, 8, 0x0010, 0},
    {"NumberValue", 24, 4, 0x0008, 6089},
    {"Increment", 28, 1, 0x0014, 1492},
};

inline constexpr Field kFields_01CC[] = {
    {"Criteria", 8, 12, 0x0024, 302},
    {"Behaviors", 24, 12, 0x0024, 303},
};

inline constexpr Field kFields_01CD[] = {
    {"Identifier", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_01CF[] = {
    {"DesignerFlags", 0, 12, 0x0024, 304},
    {"TitleLams", 12, 4, 0x0000, 424},
    {"BehaviorSets", 16, 12, 0x0024, 305},
    {"DescriptionLams", 28, 4, 0x0000, 425},
    {"ChildList", 32, 12, 0x0024, 306},
    {"CodeFlags", 44, 2, 0x0204, 802},
    {"CompletionType", 46, 1, 0x0105, 803},
    {"ActivateCriteria", 48, 12, 0x0024, 307},
    {"CompletionCriteria", 64, 12, 0x0024, 308},
    {"UniqueName", 80, 8, 0x0018, 0},
};

inline constexpr Field kFields_01D0[] = {
    {"Markers", 88, 12, 0x0024, 314},
    {"FailureCriteria", 104, 12, 0x0024, 315},
};

inline constexpr Field kFields_01D1[] = {
    {"CompletionCriteriaForCount", 120, 4, 0x0000, 430},
};

inline constexpr Field kFields_01D3[] = {
    {"MaterialSwap", 88, 8, 0x0010, 0},
};

inline constexpr Field kFields_01D4[] = {
    {"Categories", 0, 12, 0x0024, 335},
    {"HeaderLams", 12, 4, 0x0000, 435},
    {"FlagsHasAll", 16, 12, 0x0024, 336},
    {"BodyLams", 28, 4, 0x0000, 436},
    {"FlagsHasAny", 32, 12, 0x0024, 337},
    {"FlagsHasNone", 48, 12, 0x0024, 338},
    {"Name", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_01D5[] = {
    {"QuestList", 0, 12, 0x0024, 339},
    {"Categories", 16, 12, 0x0024, 340},
    {"HardSaveFacts", 32, 12, 0x0024, 341},
    {"SnapshotEtherealFacts", 48, 12, 0x0024, 342},
    {"SnapshotRelevantVSGlobalVars", 64, 12, 0x0024, 343},
};

inline constexpr Field kFields_01D6[] = {
    {"TemplateName", 0, 8, 0x0010, 0},
    {"DefaultGOName", 8, 8, 0x0010, 0},
    {"GameObjectBehaviors", 16, 12, 0x0024, 344},
    {"AllowInMenus", 28, 1, 0x0014, 1493},
};

inline constexpr Field kFields_01D7[] = {
    {"RequiresBehaviorList", 0, 12, 0x0024, 345},
    {"Type", 12, 1, 0x0105, 813},
    {"Enabled", 13, 1, 0x0014, 1494},
    {"PauseFlags", 14, 1, 0x0204, 814},
};

inline constexpr Field kFields_01D8[] = {
    {"ChildAttachJoint", 16, 8, 0x0010, 0},
    {"SynchDefaultAnim", 24, 8, 0x0010, 0},
    {"HideJoints", 32, 12, 0x0024, 347},
    {"Flags", 44, 1, 0x0204, 818},
};

inline constexpr Field kFields_01D9[] = {
    {"DebugText", 16, 8, 0x0018, 0},
    {"DebugRadius", 24, 4, 0x0008, 6090},
};

inline constexpr Field kFields_01DA[] = {
    {"PhysicsJoint", 16, 8, 0x0010, 243},
    {"InitialPhysicsState", 24, 1, 0x0104, 825},
};

inline constexpr Field kFields_01DB[] = {
    {"InitialMove", 16, 8, 0x001C, 1283},
    {"InitialFlags", 24, 1, 0x0204, 829},
    {"AllowEmptyState", 25, 1, 0x0014, 1499},
    {"UpdateActionsWhileHidden", 26, 1, 0x0014, 1500},
};

inline constexpr Field kFields_01DC[] = {
    {"val_4", 16, 1, 0x0104, 833},
    {"Attachments", 24, 8, 0x001C, 1138},
};

inline constexpr Field kFields_01DD[] = {
    {"WeaponTrailJointData", 16, 8, 0x001C, 1137},
};

inline constexpr Field kFields_01DE[] = {
    {"WeaponRayCastList", 16, 12, 0x0024, 354},
};

inline constexpr Field kFields_01DF[] = {
    {"ShowJoints", 0, 12, 0x0024, 355},
    {"WeaponLevel", 12, 4, 0x0000, 437},
    {"PlaySoundNames", 16, 12, 0x0024, 356},
    {"Modifiers", 32, 12, 0x0024, 357},
    {"WeaponTrail", 48, 12, 0x0024, 358},
};

inline constexpr Field kFields_01E0[] = {
    {"WeaponUseSlot", 16, 12, 0x0024, 360},
    {"WeaponType", 28, 4, 0x0000, 439},
    {"InitialShowJoints", 32, 12, 0x0024, 361},
    {"WeaponGroup", 44, 4, 0x0000, 440},
    {"StateMoves", 48, 12, 0x0024, 362},
    {"val_6", 60, 4, 0x0000, 441},
    {"LinkedWeapon", 64, 12, 0x0024, 363},
    {"WeaponMaxRegenCount", 76, 4, 0x0000, 443},
    {"WeaponLevels", 80, 12, 0x0024, 364},
    {"WeaponHeap", 92, 4, 0x0000, 444},
    {"MaxClonesConditions", 96, 12, 0x0024, 365},
    {"WeaponBaseScale", 108, 4, 0x0008, 6091},
    {"DefaultActiveJoint", 112, 8, 0x0018, 0},
    {"DefaultStowJoint", 120, 8, 0x0018, 0},
    {"HandEquippedAnim", 128, 8, 0x0010, 0},
    {"RecycleConcussion", 136, 8, 0x001C, 268},
    {"ShatterConcussion", 144, 8, 0x001C, 268},
    {"ReticleTargetGroup", 152, 8, 0x0010, 0},
    {"UXMessageWeaponName", 160, 8, 0x0018, 0},
    {"WeaponStowScale", 168, 4, 0x0008, 6092},
    {"MaxTimeBeforeAttach", 172, 4, 0x0008, 6093},
    {"TimeToRespawn", 176, 4, 0x0008, 6094},
    {"DespawnDelay", 180, 4, 0x0008, 6095},
    {"ShatterDespawnDelay", 184, 4, 0x0008, 6096},
    {"DefaultWeaponLevel", 188, 4, 0x0000, 445},
    {"WeaponMaxAmmo", 192, 4, 0x0000, 446},
    {"ActiveWeaponPriority", 196, 4, 0x0000, 447},
    {"CloneOfWeaponType", 200, 4, 0x0000, 448},
    {"ReticleRadius", 204, 4, 0x0008, 6097},
    {"ShatterOnThrowStates", 208, 2, 0x0204, 843},
    {"val_5", 210, 1, 0x0104, 844},
    {"DropOnStow", 211, 1, 0x0014, 1505},
    {"HideOnStow", 212, 1, 0x0014, 1506},
    {"IsDefaultForSlot", 213, 1, 0x0014, 1507},
    {"IsActiveWhenThrown", 214, 1, 0x0014, 1508},
    {"InitialWeaponState", 215, 1, 0x0104, 845},
    {"AllowsEmbedMessages", 216, 1, 0x0014, 1509},
    {"InheritsJointScale", 217, 1, 0x0014, 1510},
    {"RespawnOnThrown", 218, 1, 0x0014, 1511},
    {"CanDrop", 219, 1, 0x0014, 1512},
    {"AutoSubStepCollision", 220, 1, 0x0014, 1513},
    {"ShouldSave", 221, 1, 0x0014, 1514},
};

inline constexpr Field kFields_01E1[] = {
    {"Type", 0, 1, 0x0105, 846},
    {"ChildList", 8, 12, 0x0024, 366},
    {"Delay", 20, 4, 0x0008, 6098},
};

inline constexpr Field kFields_01E2[] = {
    {"Frequency", 24, 4, 0x0008, 6100},
    {"Amplitude", 28, 4, 0x0008, 6101},
    {"Pan", 32, 4, 0x0008, 6102},
};

inline constexpr Field kFields_01E3[] = {
    {"WaveType", 40, 1, 0x0104, 849},
};

inline constexpr Field kFields_01E4[] = {
    {"NumPulses", 48, 4, 0x0008, 6111},
};

inline constexpr Field kFields_01E5[] = {
    {"Duration", 48, 4, 0x0008, 6116},
};

inline constexpr Field kFields_01E6[] = {
    {"Duration", 40, 4, 0x0008, 6121},
};

inline constexpr Field kFields_01E7[] = {
    {"Duration", 24, 4, 0x0008, 6123},
    {"Frequency", 28, 4, 0x0008, 6124},
    {"RandomDelay", 32, 4, 0x0008, 6125},
};

inline constexpr Field kFields_01E8[] = {
    {"Duration", 24, 4, 0x0008, 6127},
    {"Attack", 28, 4, 0x0008, 6128},
    {"Decay", 32, 4, 0x0008, 6129},
    {"Sustain", 36, 4, 0x0008, 6130},
    {"Release", 40, 4, 0x0008, 6131},
};

inline constexpr Field kFields_01E9[] = {
    {"ParamType", 0, 1, 0x0105, 857},
};

inline constexpr Field kFields_01EA[] = {
    {"ParamName", 8, 8, 0x0010, 0},
    {"ParamMin", 16, 4, 0x0008, 6132},
    {"ParamMax", 20, 4, 0x0008, 6133},
    {"ResultMin", 24, 4, 0x0008, 6134},
    {"ResultMax", 28, 4, 0x0008, 6135},
};

inline constexpr Field kFields_01EB[] = {
    {"KeyframeList", 8, 12, 0x0024, 374},
    {"Loop", 20, 1, 0x0014, 1515},
};

inline constexpr Field kFields_01EC[] = {
    {"Min", 4, 4, 0x0008, 6136},
    {"Max", 8, 4, 0x0008, 6137},
};

inline constexpr Field kFields_01ED[] = {
    {"AmplitudeParam", 24, 8, 0x001C, 489},
    {"PanParam", 32, 8, 0x001C, 489},
    {"FrequencyMultiplierParam", 40, 8, 0x001C, 489},
};

inline constexpr Field kFields_01EE[] = {
    {"Type", 0, 1, 0x0105, 862},
    {"Control", 1, 1, 0x0104, 863},
    {"AllowedTrigger", 2, 1, 0x0204, 864},
    {"Priority", 4, 4, 0x0000, 449},
};

inline constexpr Field kFields_01EF[] = {
    {"Position", 8, 1, 0x0000, 451},
    {"Strength", 9, 1, 0x0000, 452},
};

inline constexpr Field kFields_01F0[] = {
    {"StartPosition", 8, 1, 0x0000, 454},
    {"EndPosition", 9, 1, 0x0000, 455},
    {"StartStrength", 10, 1, 0x0000, 456},
    {"EndStrength", 11, 1, 0x0000, 457},
};

inline constexpr Field kFields_01F1[] = {
    {"StrengthList", 8, 10, 0x0024, 376},
};

inline constexpr Field kFields_01F2[] = {
    {"StartPosition", 8, 1, 0x0000, 461},
    {"EndPosition", 9, 1, 0x0000, 462},
    {"Strength", 10, 1, 0x0000, 463},
};

inline constexpr Field kFields_01F3[] = {
    {"Position", 8, 1, 0x0000, 465},
    {"Amplitude", 9, 1, 0x0000, 466},
    {"Frequency", 10, 1, 0x0004, 880},
};

inline constexpr Field kFields_01F4[] = {
    {"AmplitudeList", 8, 10, 0x0024, 377},
    {"Frequency", 18, 1, 0x0004, 884},
};

inline constexpr Field kFields_01F5[] = {
    {"ArrowEmitter", 8, 8, 0x001C, 411},
    {"Arrow", 16, 8, 0x001C, 410},
    {"UseBlackboardVariables", 24, 1, 0x0014, 1516},
    {"TargetGameObjectVariable", 32, 8, 0x0010, 244},
    {"TargetJointVariable", 40, 8, 0x0010, 245},
    {"NoTargetLocationVariable", 48, 8, 0x0010, 246},
    {"CreatorEmitJointVariable", 56, 8, 0x0010, 247},
    {"PlayerCommandVariable", 64, 8, 0x0010, 248},
};

inline constexpr Field kFields_01F6[] = {
    {"OrbEmitter", 8, 8, 0x001C, 321},
};

inline constexpr Field kFields_01F7[] = {
    {"BanterName", 8, 8, 0x0010, 0},
    {"Critical", 16, 1, 0x0014, 1517},
    {"DisableErrorForCritical", 17, 1, 0x0014, 1518},
    {"MainCharactersConflict", 18, 1, 0x0014, 1519},
    {"ArbitationDelay", 20, 4, 0x0008, 6145},
};

inline constexpr Field kFields_01F9[] = {
    {"FactName", 8, 8, 0x0010, 0},
    {"Value", 16, 4, 0x0008, 6150},
    {"Duration", 20, 4, 0x0008, 6151},
    {"Infinite", 24, 1, 0x0014, 1520},
    {"Encounter", 25, 1, 0x0014, 1521},
};

inline constexpr Field kFields_01FA[] = {
    {"FactName", 8, 8, 0x0010, 0},
    {"Value", 16, 8, 0x0010, 0},
    {"Duration", 24, 4, 0x0008, 6154},
    {"Infinite", 28, 1, 0x0014, 1522},
    {"Encounter", 29, 1, 0x0014, 1523},
};

inline constexpr Field kFields_01FB[] = {
    {"FactName", 8, 8, 0x0010, 0},
    {"Value", 16, 4, 0x0008, 6157},
    {"Duration", 20, 4, 0x0008, 6158},
    {"Infinite", 24, 1, 0x0014, 1524},
    {"Encounter", 25, 1, 0x0014, 1525},
    {"UseNewDuration", 26, 1, 0x0014, 1526},
};

inline constexpr Field kFields_01FC[] = {
    {"MarkerID", 8, 8, 0x0010, 0},
    {"JointName", 16, 8, 0x0010, 0},
    {"PointTest", 24, 1, 0x0014, 1527},
};

inline constexpr Field kFields_01FD[] = {
    {"MarkerID", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_01FE[] = {
    {"NewBoatDockingState", 8, 1, 0x0104, 915},
};

inline constexpr Field kFields_01FF[] = {
    {"TemplateSymbol", 0, 8, 0x001A, 0},
    {"TimeAfterDeathToDespawnSec", 8, 4, 0x0008, 6165},
    {"DeathEffectAndDespawnDelaySec", 12, 4, 0x0008, 6166},
    {"ObservedDeathMovePercent", 16, 2, 0x0008, 6167},
    {"DeathEffectStartPercent", 18, 2, 0x0008, 6168},
    {"AutomaticDeathTriggerPercent", 20, 2, 0x0008, 6169},
    {"ObservedDeathMovePercent_IsNull", 22, 1, 0x0016, 1528},
    {"DeathEffectStartPercent_IsNull", 23, 1, 0x0016, 1529},
    {"AutomaticDeathTriggerPercent_IsNull", 24, 1, 0x0016, 1530},
    {"TimeAfterDeathToDespawnSec_IsNull", 25, 1, 0x0016, 1531},
    {"DeathEffectAndDespawnDelaySec_IsNull", 26, 1, 0x0016, 1532},
    {"DeathEffectIndex", 27, 1, 0x0000, 479},
    {"DeathEffectIndex_IsNull", 28, 1, 0x0016, 1533},
    {"AutomaticDeath", 29, 1, 0x0014, 1534},
    {"AutomaticDeath_IsNull", 30, 1, 0x0016, 1535},
    {"PersistentCorpse", 31, 1, 0x0014, 1536},
    {"PersistentCorpse_IsNull", 32, 1, 0x0016, 1537},
    {"NoDying", 33, 1, 0x0014, 1538},
    {"NoDying_IsNull", 34, 1, 0x0016, 1539},
    {"AllowEffectWhileDying", 35, 1, 0x0014, 1540},
    {"AllowEffectWhileDying_IsNull", 36, 1, 0x0016, 1541},
};

inline constexpr Field kFields_0200[] = {
    {"DeathParameters", 8, 0, 0x002C, 511},
    {"KillInstantly", 48, 1, 0x0014, 1556},
    {"KillInstantlyNoGameplay", 49, 1, 0x0014, 1557},
};

inline constexpr Field kFields_0201[] = {
    {"Duration", 8, 2, 0x0008, 6179},
};

inline constexpr Field kFields_0202[] = {
    {"Duration", 8, 2, 0x0008, 6182},
};

inline constexpr Field kFields_0203[] = {
    {"Name", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0204[] = {
    {"HookName", 8, 8, 0x0018, 0},
    {"SendToUI", 16, 1, 0x0015, 1558},
};

inline constexpr Field kFields_0206[] = {
    {"HookName", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_0207[] = {
    {"Context", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0208[] = {
    {"AttributeStatus", 8, 8, 0x001C, 1280},
    {"HitFlags", 16, 8, 0x0204, 944},
    {"ExcludeHitFlags", 24, 8, 0x0204, 945},
    {"PartMask", 32, 8, 0x0204, 946},
    {"ExposedSide", 40, 8, 0x0010, 0},
    {"ResultAction", 48, 1, 0x0204, 947},
    {"DeflectData", 56, 8, 0x001C, 421},
    {"GrabJoint", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_0209[] = {
    {"HitFlags", 8, 8, 0x0204, 951},
};

inline constexpr Field kFields_020A[] = {
    {"Info1", 8, 8, 0x0018, 250},
    {"Info2", 16, 8, 0x0018, 251},
};

inline constexpr Field kFields_020B[] = {
    {"JointName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_020E[] = {
    {"Bifrost", 24, 8, 0x001C, 272},
};

inline constexpr Field kFields_020F[] = {
    {"HeadTrackingID", 8, 2, 0x0008, 6209},
    {"Amount", 10, 2, 0x0008, 6210},
    {"ParentJoint", 12, 1, 0x0000, 497},
    {"POIFlags", 13, 1, 0x0204, 971},
};

inline constexpr Field kFields_0210[] = {
    {"Amount", 8, 2, 0x0008, 6213},
    {"Duration", 10, 2, 0x0008, 6214},
};

inline constexpr Field kFields_0211[] = {
    {"BindingKey", 8, 8, 0x0010, 0},
    {"AnimName", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0212[] = {
    {"SetFlags", 8, 8, 0x0204, 981},
};

inline constexpr Field kFields_0213[] = {
    {"SynchFlags", 8, 4, 0x0204, 985},
    {"MasterSynchJoint", 16, 8, 0x0010, 0},
    {"SlaveSynchJoint", 24, 8, 0x0010, 0},
    {"SlaveMoveName", 32, 8, 0x0010, 0},
    {"MasterSynchJointRotation", 40, 8, 0x0010, 0},
    {"SlaveSynchJointRotation", 48, 8, 0x0010, 0},
};

inline constexpr Field kFields_0214[] = {
    {"GroupFlags", 8, 1, 0x0204, 989},
};

inline constexpr Field kFields_0215[] = {
    {"PartMaskFlags", 8, 1, 0x0204, 993},
    {"PartMask", 16, 8, 0x0204, 994},
};

inline constexpr Field kFields_0216[] = {
    {"DampingID", 8, 1, 0x0104, 998},
    {"Level", 9, 1, 0x0104, 999},
    {"EnableINT8AutoAimBugFix", 10, 1, 0x0014, 1560},
    {"SyncDampingOnTransitionFix", 11, 1, 0x0014, 1561},
    {"AimDampingOverrideAmount", 12, 4, 0x0008, 6227},
    {"SynchDampingOverrideAmount", 16, 4, 0x0008, 6228},
};

inline constexpr Field kFields_0217[] = {
    {"SafetyCode", 24, 8, 0x0018, 0},
    {"OverrideLevel", 32, 1, 0x0104, 1004},
    {"DampFactor", 36, 4, 0x0008, 6231},
};

inline constexpr Field kFields_0218[] = {
    {"Offset", 8, 0, 0x002C, 6},
    {"Intensity", 16, 4, 0x0008, 6237},
};

inline constexpr Field kFields_0219[] = {
    {"FootEffect", 8, 2, 0x0104, 1011},
    {"JointName", 16, 8, 0x0010, 0},
    {"SoundEvent", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_021A[] = {
    {"ArgIntList", 8, 12, 0x0024, 378},
    {"ArgInt", 20, 4, 0x0000, 510},
    {"ArgString", 24, 8, 0x0010, 0},
    {"EffectFlags", 32, 1, 0x0204, 1015},
};

inline constexpr Field kFields_021B[] = {
    {"CliffEffect", 8, 1, 0x0104, 1019},
};

inline constexpr Field kFields_021C[] = {
    {"LayerList", 8, 12, 0x0024, 379},
};

inline constexpr Field kFields_021D[] = {
    {"TweenIn", 24, 0, 0x002C, 12},
    {"TweenOut", 48, 0, 0x002C, 12},
    {"Duration", 72, 4, 0x0008, 6260},
    {"IgnoreRepeats", 76, 1, 0x0014, 1562},
    {"TweenOutAtOff", 77, 1, 0x0014, 1563},
    {"Effect", 80, 8, 0x001C, 46},
    {"EffectName", 88, 8, 0x0018, 0},
    {"Weight", 96, 4, 0x0008, 6261},
    {"Priority", 100, 4, 0x0000, 515},
};

inline constexpr Field kFields_0220[] = {
    {"Effect", 8, 8, 0x001C, 46},
    {"EffectName", 16, 8, 0x0018, 0},
    {"TweenOut", 24, 0, 0x002C, 12},
};

inline constexpr Field kFields_0223[] = {
    {"Name", 8, 8, 0x0018, 0},
    {"Time", 16, 4, 0x0008, 6336},
};

inline constexpr Field kFields_0224[] = {
    {"FactName", 8, 8, 0x0010, 0},
    {"FactValue", 16, 4, 0x0008, 6339},
};

inline constexpr Field kFields_0225[] = {
    {"FactName", 8, 8, 0x0010, 0},
    {"FactValue", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0226[] = {
    {"FactName", 8, 8, 0x0010, 0},
    {"FactValue", 16, 4, 0x0008, 6344},
};

inline constexpr Field kFields_0227[] = {
    {"EventName", 8, 8, 0x0010, 0},
    {"Target", 16, 1, 0x0104, 1061},
    {"Server", 17, 1, 0x0204, 1062},
    {"SuppressWhenPaused", 18, 1, 0x0014, 1572},
};

inline constexpr Field kFields_0228[] = {
    {"QTEType", 8, 8, 0x0018, 0},
    {"Control", 16, 1, 0x0104, 1066},
    {"PosX", 20, 4, 0x0008, 6349},
    {"PosY", 24, 4, 0x0008, 6350},
    {"HoldMeterName", 32, 8, 0x0018, 0},
    {"Text", 40, 8, 0x0018, 0},
    {"Lams", 48, 4, 0x0000, 531},
    {"ControlOverride", 52, 4, 0x0000, 532},
};

inline constexpr Field kFields_022A[] = {
    {"EventName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_022C[] = {
    {"DriverName", 24, 8, 0x0010, 0},
    {"RotationSeconds", 32, 4, 0x0008, 6359},
};

inline constexpr Field kFields_022D[] = {
    {"MinPlayRate", 24, 4, 0x0008, 6362},
    {"MaxPlayRate", 28, 4, 0x0008, 6363},
    {"JumpDistance", 32, 4, 0x0008, 6364},
    {"MinDistanceFromLedge", 36, 4, 0x0008, 6365},
    {"ControlAngleRange", 40, 4, 0x0008, 6366},
    {"LeftStickInfluence", 44, 4, 0x0008, 6367},
    {"TargetDirectionType", 48, 1, 0x0104, 1084},
};

inline constexpr Field kFields_022E[] = {
    {"Amount", 24, 2, 0x0008, 6370},
    {"Duration", 26, 2, 0x0008, 6371},
    {"EaseInOutTime", 28, 4, 0x0008, 6372},
    {"AffectPlayer", 32, 1, 0x0014, 1573},
    {"ClipToMove", 33, 1, 0x0014, 1574},
};

inline constexpr Field kFields_022F[] = {
    {"SlowPlayer", 80, 1, 0x0014, 1576},
    {"SlowUI", 81, 1, 0x0014, 1577},
    {"Amount", 84, 4, 0x0008, 6387},
};

inline constexpr Field kFields_0230[] = {
    {"Amount", 24, 2, 0x0008, 6390},
    {"Duration", 26, 2, 0x0008, 6391},
    {"SlowPlayer", 28, 1, 0x0014, 1578},
    {"SlowUI", 29, 1, 0x0014, 1579},
    {"TweenIn", 32, 0, 0x002C, 12},
    {"TweenOut", 56, 0, 0x002C, 12},
};

inline constexpr Field kFields_0231[] = {
    {"Orb", 24, 8, 0x001C, 320},
    {"AttractJointName", 32, 8, 0x0010, 0},
    {"EmitJointName", 40, 8, 0x0010, 0},
    {"DistributionRadius", 48, 4, 0x0008, 6406},
    {"AttractYOffset", 52, 4, 0x0008, 6407},
    {"AttractZOffset", 56, 4, 0x0008, 6408},
    {"OrbsToEmit", 60, 2, 0x0000, 543},
    {"EmitLocation", 62, 1, 0x0104, 1099},
    {"EmitPattern", 63, 1, 0x0104, 1100},
};

inline constexpr Field kFields_0232[] = {
    {"DisableCollision", 24, 1, 0x0014, 1580},
};

inline constexpr Field kFields_0233[] = {
    {"Params", 24, 8, 0x001C, 385},
};

inline constexpr Field kFields_0234[] = {
    {"OverrideFacingType", 24, 1, 0x0104, 1113},
    {"MinAngle", 28, 4, 0x0008, 6415},
    {"MaxAngle", 32, 4, 0x0008, 6416},
    {"FlipDirectionTolerance", 36, 4, 0x0008, 6417},
};

inline constexpr Field kFields_0235[] = {
    {"OverrideTargetDirReference", 24, 1, 0x0104, 1118},
    {"Scale", 28, 4, 0x0008, 6420},
    {"MinAngle", 32, 4, 0x0008, 6421},
    {"MaxAngle", 36, 4, 0x0008, 6422},
    {"ReverseTurnMaxAngle", 40, 4, 0x0008, 6423},
    {"AngleDamping", 44, 4, 0x0008, 6424},
    {"FanOutStartPercent", 48, 4, 0x0008, 6425},
    {"FanOutEndPercent", 52, 4, 0x0008, 6426},
    {"Offset", 56, 4, 0x0008, 6427},
};

inline constexpr Field kFields_0238[] = {
    {"MinAngle", 24, 4, 0x0008, 6434},
    {"MaxAngle", 28, 4, 0x0008, 6435},
};

inline constexpr Field kFields_0239[] = {
    {"MinAngleLeft", 24, 4, 0x0008, 6438},
    {"MaxAngleLeft", 28, 4, 0x0008, 6439},
    {"MinAngleRight", 32, 4, 0x0008, 6440},
    {"MaxAngleRight", 36, 4, 0x0008, 6441},
    {"MinRotationTime", 40, 4, 0x0008, 6442},
    {"MaxRotationTime", 44, 4, 0x0008, 6443},
};

inline constexpr Field kFields_023B[] = {
    {"CoolDownTimeMin", 24, 4, 0x0008, 6448},
    {"CoolDownTimeMax", 28, 4, 0x0008, 6449},
};

inline constexpr Field kFields_023C[] = {
    {"Target", 24, 4, 0x0008, 6452},
    {"Speed", 28, 4, 0x0008, 6453},
    {"Damping", 32, 4, 0x0008, 6454},
    {"Frequency", 36, 4, 0x0008, 6455},
    {"Amplitude", 40, 4, 0x0008, 6456},
};

inline constexpr Field kFields_0241[] = {
    {"EnterFollowType", 24, 1, 0x0104, 1167},
};

inline constexpr Field kFields_0242[] = {
    {"ExitFollowType", 24, 1, 0x0104, 1172},
};

inline constexpr Field kFields_0243[] = {
    {"DragonCenterOffset", 24, 0, 0x002C, 7},
    {"PushOrPull", 36, 4, 0x0000, 562},
    {"FromDragonCenterOrNegativeZAxis", 40, 4, 0x0000, 563},
    {"Distance", 44, 4, 0x0008, 6474},
    {"Radius", 48, 4, 0x0008, 6475},
    {"MinOffset", 52, 4, 0x0008, 6476},
};

inline constexpr Field kFields_0244[] = {
    {"UseLocalXZOnly", 24, 1, 0x0014, 1581},
    {"LocalX", 28, 4, 0x0008, 6479},
    {"LocalY", 32, 4, 0x0008, 6480},
    {"LocalZ", 36, 4, 0x0008, 6481},
};

inline constexpr Field kFields_0245[] = {
    {"SpeedList", 24, 12, 0x0024, 380},
    {"SpeedPercentList", 40, 12, 0x0024, 381},
};

inline constexpr Field kFields_0247[] = {
    {"InterpolateY", 24, 1, 0x0014, 1582},
};

inline constexpr Field kFields_0248[] = {
    {"MPIconName", 24, 8, 0x0010, 0},
    {"HintDistance", 32, 4, 0x0008, 6492},
    {"ShowDistance", 36, 4, 0x0008, 6493},
    {"MaxFacingToObjectTolerance", 40, 4, 0x0008, 6494},
    {"MaxFacingToDirectionTolerance", 44, 4, 0x0008, 6495},
    {"MaxInFrontOfObjectAngleTolerance", 48, 4, 0x0008, 6496},
    {"PromptOffset", 52, 0, 0x002C, 6},
    {"PromptAtEndPosition", 58, 1, 0x0014, 1583},
    {"TraversalMove", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_0249[] = {
    {"Prompt", 24, 12, 0x0024, 382},
};

inline constexpr Field kFields_024A[] = {
    {"ShowHint", 24, 1, 0x0014, 1584},
};

inline constexpr Field kFields_024B[] = {
    {"TraversalMove", 24, 12, 0x0024, 383},
    {"MinimumIntent", 36, 4, 0x0008, 6506},
    {"TraversalExitMove", 40, 12, 0x0024, 384},
    {"MinOnScreenPercent", 52, 4, 0x0008, 6507},
    {"MPIconName", 56, 8, 0x0010, 0},
    {"MPIconOffscreenName", 64, 8, 0x0010, 0},
    {"PromptOffset", 72, 0, 0x002C, 6},
    {"ExitPromptOffset", 78, 0, 0x002C, 6},
};

inline constexpr Field kFields_024C[] = {
    {"ZiplineObjectName", 24, 8, 0x0018, 0},
    {"TransitionTime", 32, 4, 0x0008, 6516},
};

inline constexpr Field kFields_024D[] = {
    {"GroundPoundLandingZoneScale", 24, 4, 0x0008, 6519},
    {"GroundPoundTargetOffset", 28, 0, 0x002C, 6},
    {"MaxAttackDistance", 36, 4, 0x0008, 6523},
    {"UseCreatureTargeting", 40, 1, 0x0014, 1585},
    {"TargetCreaturePositionWeight", 44, 4, 0x0008, 6524},
    {"GroundPoundWithTargetMaxHorizontalDelta", 48, 4, 0x0008, 6525},
    {"GroundPoundNoTargetMaxHorizontalDelta", 52, 4, 0x0008, 6526},
};

inline constexpr Field kFields_024F[] = {
    {"WeaponModeList", 24, 12, 0x0024, 385},
    {"EnterDistance", 36, 4, 0x0008, 6531},
    {"WeaponMode", 40, 8, 0x0010, 0},
    {"ShortRangeApproachDistance", 48, 4, 0x0008, 6532},
    {"TweenTimeToEnter", 52, 4, 0x0008, 6533},
    {"TimeOutTime", 56, 4, 0x0008, 6534},
    {"MarkWeapon", 60, 1, 0x0014, 1586},
};

inline constexpr Field kFields_0250[] = {
    {"WeaponMode", 24, 8, 0x0010, 0},
    {"MarkWeapon", 32, 1, 0x0014, 1587},
};

inline constexpr Field kFields_0252[] = {
    {"Amplitude", 24, 4, 0x0008, 6541},
    {"Frequency", 28, 4, 0x0008, 6542},
    {"BreathingTweenTime", 32, 4, 0x0008, 6543},
};

inline constexpr Field kFields_0253[] = {
    {"Slot", 24, 4, 0x0000, 580},
    {"PlayerEnteringBoatFromCheckpoint", 28, 1, 0x0014, 1588},
};

inline constexpr Field kFields_0254[] = {
    {"DisableCreatureVehicleIK", 24, 1, 0x0014, 1589},
    {"DisableWeaponAdjustments", 25, 1, 0x0014, 1590},
    {"DisablePassengerParticleIK", 26, 1, 0x0014, 1591},
};

inline constexpr Field kFields_0256[] = {
    {"BranchName", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0257[] = {
    {"BranchName", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_025D[] = {
    {"TransitionMultiplier", 24, 4, 0x0008, 6566},
};

inline constexpr Field kFields_025E[] = {
    {"DelayAfterCachingDisabled", 24, 4, 0x0008, 6569},
};

inline constexpr Field kFields_025F[] = {
    {"ClosestDistanceFromCamera", 24, 4, 0x0008, 6572},
    {"FarthestDistanceFromCamera", 28, 4, 0x0008, 6573},
    {"SearchInterval", 32, 4, 0x0008, 6574},
    {"OffsetDistanceFromFrustum", 36, 4, 0x0008, 6575},
    {"SearchOrder", 40, 1, 0x0104, 1289},
    {"NavmeshRaycastToPlayer", 41, 1, 0x0014, 1592},
    {"RaycastToPlayer", 42, 1, 0x0014, 1593},
    {"NavmeshRaycastToTarget", 43, 1, 0x0014, 1594},
    {"RaycastToTarget", 44, 1, 0x0014, 1595},
    {"ThresholdToCenterPlane", 48, 4, 0x0008, 6576},
};

inline constexpr Field kFields_0260[] = {
    {"TargetJointName", 24, 8, 0x0010, 0},
    {"ClosestJointName", 32, 8, 0x0010, 0},
    {"FurthestJointName", 40, 8, 0x0010, 0},
    {"TargetOffset", 48, 0, 0x002C, 6},
    {"StopStretchingTime", 54, 2, 0x0008, 6582},
    {"Length", 56, 4, 0x0008, 6583},
    {"TargetExtraLength", 60, 4, 0x0008, 6584},
    {"MaxLength", 64, 4, 0x0008, 6585},
    {"MinLength", 68, 4, 0x0008, 6586},
    {"StartRetractingTime", 72, 2, 0x0008, 6587},
    {"UseAnim", 74, 1, 0x0014, 1596},
    {"IncludeClosestNodeSiblings", 75, 1, 0x0014, 1597},
    {"StretchDirection", 76, 1, 0x0104, 1294},
    {"StretchType", 77, 1, 0x0104, 1295},
};

inline constexpr Field kFields_0261[] = {
    {"Throttle", 24, 0, 0x002C, 1511},
    {"Rotation", 40, 0, 0x002C, 1511},
    {"AccelerationScaleCurveValueList", 56, 12, 0x0024, 386},
    {"DefaultAccelerationScale", 68, 4, 0x0008, 6593},
    {"AccelerationScaleCurveTimingList", 72, 12, 0x0024, 387},
};

inline constexpr Field kFields_0263[] = {
    {"DampingScale", 24, 4, 0x0008, 6599},
};

inline constexpr Field kFields_0264[] = {
    {"Duration", 24, 2, 0x0008, 6602},
    {"ResetTimerAcrossMoves", 26, 1, 0x0014, 1598},
};

inline constexpr Field kFields_0266[] = {
    {"Amount", 24, 2, 0x0008, 6607},
};

inline constexpr Field kFields_0267[] = {
    {"PhaseID", 24, 1, 0x0000, 601},
};

inline constexpr Field kFields_0268[] = {
    {"Overlay", 24, 8, 0x001C, 246},
};

inline constexpr Field kFields_0269[] = {
    {"MaterialConstantName", 24, 8, 0x0018, 0},
    {"Value", 32, 4, 0x0008, 6614},
};

inline constexpr Field kFields_026A[] = {
    {"Team", 24, 1, 0x0104, 1338},
};

inline constexpr Field kFields_026B[] = {
    {"MinCombatDistance", 24, 2, 0x0008, 6619},
};

inline constexpr Field kFields_026C[] = {
    {"Stance", 24, 8, 0x001C, 328},
};

inline constexpr Field kFields_026D[] = {
    {"DistanceThreshold", 24, 4, 0x0008, 6624},
    {"WorldOffsetY", 28, 4, 0x0008, 6625},
};

inline constexpr Field kFields_026E[] = {
    {"EventType", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_026F[] = {
    {"DeleteEntity", 32, 1, 0x0000, 610},
    {"IntArg0", 36, 4, 0x0000, 611},
    {"FloatArg1", 40, 4, 0x0008, 6630},
};

inline constexpr Field kFields_0270[] = {
    {"MarkerID", 24, 8, 0x0010, 0},
    {"JointName", 32, 8, 0x0010, 0},
    {"PointTest", 40, 1, 0x0014, 1599},
};

inline constexpr Field kFields_0271[] = {
    {"MarkerID", 24, 8, 0x0010, 0},
    {"JointName", 32, 8, 0x0010, 0},
    {"WeaponType", 40, 4, 0x0000, 614},
    {"PointTest", 44, 1, 0x0014, 1600},
};

inline constexpr Field kFields_0272[] = {
    {"WeaponType", 24, 4, 0x0000, 616},
    {"MarkerID", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_0273[] = {
    {"MarkerID", 24, 8, 0x0010, 0},
    {"JointName", 32, 8, 0x0010, 0},
    {"WeaponType", 40, 4, 0x0000, 618},
    {"PointTest", 44, 1, 0x0014, 1601},
};

inline constexpr Field kFields_0274[] = {
    {"WeaponType", 24, 4, 0x0000, 620},
    {"MaxActions", 28, 4, 0x0000, 621},
    {"MarkerID", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_0275[] = {
    {"FlagID", 24, 8, 0x0010, 0},
    {"Persist", 32, 1, 0x0014, 1602},
};

inline constexpr Field kFields_0276[] = {
    {"FlagID", 24, 8, 0x0010, 0},
    {"Persist", 32, 1, 0x0014, 1603},
};

inline constexpr Field kFields_0277[] = {
    {"FlagID", 24, 8, 0x0010, 0},
    {"Persist", 32, 1, 0x0014, 1604},
};

inline constexpr Field kFields_027A[] = {
    {"MoveType", 24, 1, 0x0104, 1403},
};

inline constexpr Field kFields_027B[] = {
    {"Parameters", 24, 0, 0x002C, 239},
    {"TargetsType", 152, 1, 0x0204, 1409},
};

inline constexpr Field kFields_027C[] = {
    {"Joint", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_027D[] = {
    {"Amount", 24, 4, 0x0008, 6687},
    {"ApplyToTargetOnly", 28, 1, 0x0014, 1605},
    {"ApplyOnlyToThisMove", 29, 1, 0x0014, 1606},
    {"RestorOriginalTimeScaleAfter", 30, 1, 0x0014, 1607},
};

inline constexpr Field kFields_027E[] = {
    {"IgnorePickupSlot", 24, 1, 0x0014, 1608},
    {"IgnoreMoveSystemSlot", 25, 1, 0x0014, 1609},
    {"IgnoreHitPauseSlot", 26, 1, 0x0014, 1610},
    {"IgnoreWorldSlowDown", 27, 1, 0x0014, 1611},
    {"IgnoreLuaSlot", 28, 1, 0x0014, 1612},
    {"IgnoreTimeBubbleSlot", 29, 1, 0x0014, 1613},
};

inline constexpr Field kFields_027F[] = {
    {"WeaponType", 24, 4, 0x0000, 633},
};

inline constexpr Field kFields_0280[] = {
    {"WeaponType", 24, 4, 0x0000, 635},
    {"TweenTime", 28, 4, 0x0008, 6694},
};

inline constexpr Field kFields_0281[] = {
    {"WeaponType", 24, 4, 0x0000, 637},
    {"SnapTarget", 28, 4, 0x0000, 638},
};

inline constexpr Field kFields_0282[] = {
    {"HorizontalOffset", 24, 4, 0x0008, 6699},
};

inline constexpr Field kFields_0284[] = {
    {"LeftStickDamping", 24, 4, 0x0008, 6704},
    {"RightStickDamping", 28, 4, 0x0008, 6705},
};

inline constexpr Field kFields_0286[] = {
    {"MeterName", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0287[] = {
    {"Amount", 24, 0, 0x002C, 229},
    {"SourceMeterName", 64, 8, 0x0010, 0},
    {"MeterName", 72, 8, 0x0010, 0},
    {"HitFlagsForDamage", 80, 8, 0x0204, 1461},
    {"Rate", 88, 4, 0x0008, 6718},
    {"Pickup", 92, 2, 0x0000, 645},
    {"MeterFlags", 94, 1, 0x0204, 1462},
    {"PauseMeter", 95, 1, 0x0104, 1463},
    {"HealthMeterUseDamagePipeline", 96, 1, 0x0014, 1614},
    {"UseStats", 97, 1, 0x0014, 1615},
    {"TriggerAttackerHitEvent", 98, 1, 0x0014, 1616},
    {"TriggerCollisionEventForHealth", 99, 1, 0x0014, 1617},
    {"TriggerMPIconCombatAnim", 100, 1, 0x0014, 1618},
    {"IgnoreInvulnerable", 101, 1, 0x0014, 1619},
    {"DamageSource", 102, 1, 0x0104, 1464},
};

inline constexpr Field kFields_0288[] = {
    {"AdjustTime", 24, 0, 0x002C, 229},
    {"ScaleTime", 64, 0, 0x002C, 229},
    {"AdjustByPercentMax", 104, 0, 0x002C, 229},
    {"MeterName", 144, 8, 0x0010, 0},
};

inline constexpr Field kFields_0289[] = {
    {"Amount", 24, 4, 0x0000, 648},
    {"CFlags", 28, 1, 0x0204, 1482},
    {"Name", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_028A[] = {
    {"EnemyContextList", 24, 12, 0x0024, 394},
    {"ExcludeActivationFlags", 36, 1, 0x0014, 1620},
    {"ExcludePartFlags", 37, 1, 0x0014, 1621},
    {"CheckAllParts", 38, 1, 0x0014, 1622},
    {"ExcludeEnemyContexts", 39, 1, 0x0014, 1623},
    {"EnemyIDList", 40, 12, 0x0024, 395},
    {"ExcludeEnemyIDs", 52, 1, 0x0014, 1624},
    {"ExcludeDynamicFlags", 53, 1, 0x0014, 1625},
    {"EnemyDynamicFlagFilterList", 56, 12, 0x0024, 396},
    {"ActivationFlags", 72, 8, 0x0204, 1487},
    {"ExcludeHitFlags", 80, 8, 0x0204, 1488},
    {"PartFlags", 88, 8, 0x0204, 1489},
    {"HitModifier", 96, 8, 0x001C, 341},
};

inline constexpr Field kFields_028B[] = {
    {"MeterName", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_028C[] = {
    {"Status", 24, 1, 0x0104, 1498},
    {"CircleID", 25, 1, 0x0000, 652},
};

inline constexpr Field kFields_028D[] = {
    {"Threshold", 24, 1, 0x0000, 654},
};

inline constexpr Field kFields_028E[] = {
    {"Position", 24, 0, 0x002C, 6},
    {"Rotation", 30, 0, 0x002C, 6},
    {"JumpHeight", 36, 4, 0x0008, 6757},
    {"Takeoff", 40, 4, 0x0008, 6758},
    {"Landing", 44, 4, 0x0008, 6759},
};

inline constexpr Field kFields_028F[] = {
    {"Choice", 24, 12, 0x0024, 397},
    {"RecoveryTime", 36, 4, 0x0008, 6762},
    {"DriverName", 40, 8, 0x0010, 353},
};

inline constexpr Field kFields_0290[] = {
    {"DriverName", 24, 8, 0x0010, 0},
    {"Value", 32, 4, 0x0008, 6765},
    {"Duration", 36, 1, 0x0104, 1515},
};

inline constexpr Field kFields_0291[] = {
    {"JointName", 24, 8, 0x0010, 0},
    {"ChildGameObjectName", 32, 8, 0x0010, 0},
    {"ChildGameObjectPartialName", 40, 8, 0x0018, 0},
    {"Status", 48, 1, 0x0104, 1520},
    {"Team", 49, 1, 0x0104, 1521},
};

inline constexpr Field kFields_0293[] = {
    {"Set", 24, 4, 0x0204, 1530},
    {"Clear", 28, 4, 0x0204, 1531},
};

inline constexpr Field kFields_0294[] = {
    {"ChildGameObjectName", 24, 8, 0x0010, 0},
    {"AnimName", 32, 8, 0x0010, 0},
    {"PlayFlags", 40, 1, 0x0204, 1536},
};

inline constexpr Field kFields_0295[] = {
    {"ConfigSpecs", 24, 12, 0x0024, 398},
};

inline constexpr Field kFields_0297[] = {
    {"NoClimbTime", 24, 4, 0x0008, 6780},
    {"DetachImpulse", 28, 0, 0x002C, 11},
};

inline constexpr Field kFields_0298[] = {
    {"Bomb", 24, 8, 0x001C, 258},
    {"TargetJointID", 32, 1, 0x0000, 666},
};

inline constexpr Field kFields_0299[] = {
    {"Stick", 24, 1, 0x0104, 1560},
    {"Adjustment", 25, 1, 0x0104, 1561},
    {"Dampening", 28, 4, 0x0008, 6790},
};

inline constexpr Field kFields_029A[] = {
    {"MaxControllerSpeed", 24, 4, 0x0008, 6793},
};

inline constexpr Field kFields_029C[] = {
    {"BlockBlendFlags", 24, 1, 0x0204, 1574},
};

inline constexpr Field kFields_029D[] = {
    {"WarpBegin", 24, 4, 0x0008, 6800},
    {"WarpEnd", 28, 4, 0x0008, 6801},
    {"Distance", 32, 4, 0x0008, 6802},
    {"IncludeStartDistance", 36, 1, 0x0014, 1626},
    {"AnimName", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_029E[] = {
    {"CharacterID", 24, 8, 0x0010, 0},
    {"BlendBegin", 32, 4, 0x0008, 6805},
    {"BlendEnd", 36, 4, 0x0008, 6806},
    {"EasingCurve", 40, 2, 0x0104, 1583},
    {"BlendType", 42, 1, 0x0104, 1584},
    {"TimeType", 43, 1, 0x0104, 1585},
};

inline constexpr Field kFields_029F[] = {
    {"DismountDirection", 24, 1, 0x0104, 1590},
    {"MaxSlopeAngle", 28, 4, 0x0008, 6809},
    {"DismountOffsetX", 32, 4, 0x0008, 6810},
    {"DismountOffsetY", 36, 4, 0x0008, 6811},
    {"DismountOffsetZ", 40, 4, 0x0008, 6812},
    {"DismountEndTime", 44, 4, 0x0008, 6813},
};

inline constexpr Field kFields_02A0[] = {
    {"StartJoint", 24, 8, 0x0010, 0},
    {"MidJoint", 32, 8, 0x0010, 0},
    {"EndJoint", 40, 8, 0x0010, 0},
    {"PoleVectorJoint", 48, 8, 0x0010, 0},
    {"TargetHelperData", 56, 8, 0x001C, 301},
    {"PoleVector", 64, 0, 0x002C, 6},
    {"BlendInComplete", 70, 2, 0x0008, 6819},
    {"MaxAngle", 72, 4, 0x0008, 6820},
    {"MaxScale", 76, 4, 0x0008, 6821},
    {"ReachT", 80, 4, 0x0008, 6822},
    {"ReachR", 84, 4, 0x0008, 6823},
    {"TargetWeight", 88, 4, 0x0008, 6824},
    {"BlendWeight", 92, 4, 0x0008, 6825},
    {"StartJointRotationBlendWeight", 96, 4, 0x0008, 6826},
    {"MidJointRotationBlendWeight", 100, 4, 0x0008, 6827},
    {"EndJointRotationBlendWeight", 104, 4, 0x0008, 6828},
    {"ScaleFlags", 108, 1, 0x0204, 1595},
};

inline constexpr Field kFields_02A2[] = {
    {"FadeTime", 24, 4, 0x0008, 6833},
    {"FadeFlags", 28, 1, 0x0204, 1604},
};

inline constexpr Field kFields_02A5[] = {
    {"JointName", 24, 8, 0x0010, 0},
    {"Distance", 32, 4, 0x0008, 6840},
    {"ConeBaseRadius", 36, 4, 0x0008, 6841},
    {"ConeTopRadius", 40, 4, 0x0008, 6842},
    {"MouthForce", 44, 4, 0x0008, 6843},
    {"DistanceForce", 48, 4, 0x0008, 6844},
    {"SonicScreamFlags", 52, 1, 0x0204, 1617},
    {"Visibility", 53, 1, 0x0204, 1618},
};

inline constexpr Field kFields_02A6[] = {
    {"JointName", 24, 8, 0x0010, 0},
    {"Radius", 32, 4, 0x0008, 6847},
    {"EdgeForce", 36, 4, 0x0008, 6848},
    {"CenterForce", 40, 4, 0x0008, 6849},
    {"Visibility", 44, 1, 0x0204, 1623},
};

inline constexpr Field kFields_02A7[] = {
    {"IndicatorFX", 24, 8, 0x001C, 272},
    {"IndicatorConcussion", 32, 8, 0x001C, 268},
    {"Concussion", 40, 8, 0x001C, 268},
    {"JointName", 48, 8, 0x0010, 0},
    {"Heap", 56, 4, 0x0000, 682},
    {"IndicatorTime", 60, 4, 0x0008, 6852},
    {"Radius", 64, 4, 0x0008, 6853},
    {"NoTargetDistance", 68, 4, 0x0008, 6854},
    {"MaxTargets", 72, 1, 0x0000, 683},
    {"PlayerFollowFlags", 73, 1, 0x0204, 1628},
};

inline constexpr Field kFields_02A8[] = {
    {"AnimName", 24, 8, 0x0010, 0},
    {"TweenTime", 32, 4, 0x0008, 6857},
};

inline constexpr Field kFields_02A9[] = {
    {"EntityName", 40, 8, 0x0018, 0},
};

inline constexpr Field kFields_02AA[] = {
    {"SwitchToGameObject", 24, 8, 0x0010, 0},
    {"TransitionTime", 32, 4, 0x0008, 6865},
    {"ResourceInLevelWad", 36, 1, 0x0014, 1627},
};

inline constexpr Field kFields_02AB[] = {
    {"RegenSpeed", 24, 4, 0x0008, 6868},
    {"NoRegenTime", 28, 4, 0x0008, 6869},
    {"HealthMaxPct", 32, 4, 0x0008, 6870},
    {"RegenEffect", 36, 2, 0x0000, 688},
    {"EndEffect", 38, 2, 0x0000, 689},
    {"SoundName", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_02AC[] = {
    {"OverrideScaleAngleList", 24, 12, 0x0024, 399},
    {"Range", 36, 4, 0x0008, 6874},
    {"OverrideScaleDistanceList", 40, 12, 0x0024, 400},
    {"HoldDistance", 52, 4, 0x0008, 6876},
    {"SourceJoint", 56, 8, 0x0010, 383},
    {"TargetJoint", 64, 8, 0x0010, 384},
    {"DisableTrackDistance", 72, 4, 0x0008, 6877},
    {"EnableTrackDistance", 76, 4, 0x0008, 6878},
    {"Speed", 80, 4, 0x0008, 6879},
    {"OrientationChange", 84, 1, 0x0014, 1628},
    {"AllowVertical", 85, 1, 0x0014, 1629},
    {"ClearFacingOnExit", 86, 1, 0x0014, 1630},
    {"TargetInitialPosition", 87, 1, 0x0014, 1631},
};

inline constexpr Field kFields_02AD[] = {
    {"SourceJoint", 24, 8, 0x0010, 386},
    {"MoveJoint", 32, 8, 0x0010, 387},
    {"TargetJoint", 40, 8, 0x0010, 388},
    {"TargetCreatureBlackboardVarName", 48, 8, 0x0010, 0},
    {"ApplyDeltaToSiblings", 56, 1, 0x0014, 1632},
    {"ApplyDeltaToSourceJoint", 57, 1, 0x0014, 1633},
    {"DampedRotation", 58, 1, 0x0014, 1634},
    {"DampedRotationAngularFrequency", 60, 4, 0x0008, 6882},
    {"DampedRotationDampingRatio", 64, 4, 0x0008, 6883},
    {"MaxDampedRotationError", 68, 4, 0x0008, 6884},
    {"ProjectRotationAlignmentOnGroundPlane", 72, 1, 0x0014, 1635},
    {"DampedTranslation", 73, 1, 0x0014, 1636},
    {"DampedTranslationAngularFrequency", 76, 4, 0x0008, 6885},
    {"DampedTranslationDampingRatio", 80, 4, 0x0008, 6886},
    {"MaxDampedTranslationError", 84, 4, 0x0008, 6887},
    {"RotationAlignmentWeight", 88, 0, 0x002C, 6},
    {"AlignTranslation", 94, 1, 0x0014, 1637},
    {"ReverseAttachment", 95, 1, 0x0014, 1638},
    {"BlendInComplete", 96, 2, 0x0008, 6891},
    {"TargetWeight", 100, 4, 0x0008, 6892},
};

inline constexpr Field kFields_02B0[] = {
    {"MoveJoint", 24, 8, 0x0010, 395},
    {"LineJointA", 32, 8, 0x0010, 396},
    {"LineJointB", 40, 8, 0x0010, 397},
    {"JointOffsetA", 48, 0, 0x002C, 6},
    {"JointOffsetB", 54, 0, 0x002C, 6},
    {"StartEnd", 60, 2, 0x0008, 6916},
    {"StopBegin", 62, 2, 0x0008, 6917},
};

inline constexpr Field kFields_02B1[] = {
    {"SourceJoint", 24, 8, 0x0010, 399},
    {"TargetJoint", 32, 8, 0x0010, 400},
    {"TargetObjectBlackboardVarName", 40, 8, 0x0010, 0},
    {"StartEnd", 48, 2, 0x0008, 6920},
    {"StopBegin", 50, 2, 0x0008, 6921},
};

inline constexpr Field kFields_02B2[] = {
    {"Joint", 24, 8, 0x0010, 402},
};

inline constexpr Field kFields_02B3[] = {
    {"Joint", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_02B4[] = {
    {"CacheJointList", 24, 8, 0x0010, 0},
    {"JointChainCacheBlackboardVarName", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_02B5[] = {
    {"JointChainAngularFrequencyData", 24, 8, 0x0010, 0},
    {"JointChainDampingRatioData", 32, 8, 0x0010, 0},
    {"BlendInComplete", 40, 2, 0x0008, 6930},
    {"TargetWeight", 44, 4, 0x0008, 6931},
};

inline constexpr Field kFields_02B6[] = {
    {"JointChainCacheBlackboardVarName", 24, 8, 0x0010, 0},
    {"BlendInComplete", 32, 2, 0x0008, 6934},
    {"TargetWeight", 36, 4, 0x0008, 6935},
};

inline constexpr Field kFields_02B7[] = {
    {"TargetHelperData", 24, 0, 0x002C, 301},
    {"PivotJointChainAngularFrequencyData", 88, 8, 0x0010, 0},
    {"PivotJointChainDampingRatioData", 96, 8, 0x0010, 0},
    {"PivotJointChainAlignPercentData", 104, 8, 0x0010, 0},
    {"PivotJointChainMaxDeltaData", 112, 8, 0x0010, 0},
    {"PivotJointChainActivationThresholdData", 120, 8, 0x0010, 0},
    {"PivotJoint", 128, 8, 0x0010, 0},
    {"SourceStartJoint", 136, 8, 0x0010, 0},
    {"SourceEndJoint", 144, 8, 0x0010, 0},
    {"ExtraChildJoint", 152, 8, 0x0010, 0},
    {"ZeroAngleJoint", 160, 8, 0x0010, 408},
    {"TrackingAnchorJoint", 168, 8, 0x0010, 0},
    {"SourceEndJointOffset", 176, 0, 0x002C, 6},
    {"ZeroAngleAxis", 182, 0, 0x002C, 6},
    {"AngularFrequency", 188, 4, 0x0008, 6953},
    {"DampingRatio", 192, 4, 0x0008, 6954},
    {"AlignPercent", 196, 4, 0x0008, 6955},
    {"MaxDelta", 200, 4, 0x0008, 6956},
    {"ActivationThreshold", 204, 4, 0x0008, 6957},
    {"CharacterRelativeTargetPositionDampingClose", 208, 4, 0x0008, 6958},
    {"CharacterRelativeTargetPositionDampingFar", 212, 4, 0x0008, 6959},
    {"CharacterRelativeTargetPositionDampingDistanceClose", 216, 4, 0x0008, 6960},
    {"CharacterRelativeTargetPositionDampingDistanceFar", 220, 4, 0x0008, 6961},
    {"TargetWeight", 224, 4, 0x0008, 6962},
    {"MinAngleToTarget", 228, 4, 0x0008, 6963},
    {"MaxAngleToTarget", 232, 4, 0x0008, 6964},
    {"TrackingRadius", 236, 4, 0x0008, 6965},
    {"TrackingMaxAngle", 240, 4, 0x0008, 6966},
    {"BlendInComplete", 244, 2, 0x0008, 6967},
    {"AlignAxis", 246, 1, 0x0104, 1698},
};

inline constexpr Field kFields_02B8[] = {
    {"PivotJoint", 24, 8, 0x0010, 0},
    {"HeightJoint", 32, 8, 0x0010, 0},
    {"NeckAlignChainDampingRatio", 40, 8, 0x0010, 410},
    {"NeckAlignChainAngularFreq", 48, 8, 0x0010, 411},
    {"NeckAlignChainMaxDelta", 56, 8, 0x0010, 412},
    {"NeckAlignChainAlignPercent", 64, 8, 0x0010, 413},
    {"NeckAlignChainActivationThreshold", 72, 8, 0x0010, 414},
    {"FireJoint", 80, 8, 0x0010, 415},
    {"BottomJawJoint", 88, 8, 0x0010, 416},
    {"NeckJoint", 96, 8, 0x0010, 417},
    {"TargetWeight", 104, 4, 0x0008, 6970},
    {"DriftLookaheadTime", 108, 4, 0x0008, 6971},
    {"FireBeamAngleSlerpPct", 112, 4, 0x0008, 6972},
    {"TargetLookaheadDist", 116, 4, 0x0008, 6973},
    {"TargetPitchDeltaDegrees", 120, 4, 0x0008, 6974},
    {"BlendInComplete", 124, 2, 0x0008, 6975},
    {"EnableProceduralPitch", 126, 1, 0x0014, 1646},
    {"NeckAlignChainUseDamping", 127, 1, 0x0014, 1647},
};

inline constexpr Field kFields_02B9[] = {
    {"OffsetAngle", 24, 4, 0x0008, 6978},
    {"RotationDriver", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_02BA[] = {
    {"PivotJoint", 0, 8, 0x0010, 0},
    {"JiggleJoint", 8, 8, 0x0010, 0},
    {"InertialJoint", 16, 8, 0x0010, 419},
    {"BoneUpdateEnabled", 24, 1, 0x0014, 1648},
    {"EnableJiggle", 25, 1, 0x0014, 1649},
    {"JiggleSpringFrequency", 28, 4, 0x0008, 6979},
    {"JiggleSpringDampingRatio", 32, 4, 0x0008, 6980},
    {"JiggleSpringMaxAngle", 36, 4, 0x0008, 6981},
    {"EnableSquashAndStretch", 40, 1, 0x0014, 1650},
    {"SquashSpringMaxDisplacement", 44, 4, 0x0008, 6982},
    {"SquashSpringFrequency", 48, 4, 0x0008, 6983},
    {"SquashSpringDampingRatio", 52, 4, 0x0008, 6984},
};

inline constexpr Field kFields_02BB[] = {
    {"ChildBones", 56, 12, 0x0024, 401},
    {"EnableHitReaction", 68, 1, 0x0014, 1654},
    {"EnableAnimationReaction", 69, 1, 0x0014, 1655},
    {"ActivationFlags", 72, 8, 0x0204, 1707},
    {"PartFlags", 80, 8, 0x0204, 1708},
};

inline constexpr Field kFields_02BD[] = {
    {"Enabled", 24, 1, 0x0014, 1656},
    {"UseZeroJointKeysDirectly", 25, 1, 0x0014, 1657},
};

inline constexpr Field kFields_02BE[] = {
    {"JiggleBones", 24, 12, 0x0024, 402},
    {"ImpactDecayStartDistance", 36, 4, 0x0008, 6997},
    {"JointsToFreeze", 40, 12, 0x0024, 403},
    {"ImpactDecayFactor", 52, 4, 0x0008, 6998},
    {"HitDirectionLeftFlags", 56, 8, 0x0204, 1721},
    {"HitDirectionRightFlags", 64, 8, 0x0204, 1722},
    {"HitDirectionUpFlags", 72, 8, 0x0204, 1723},
    {"HitDirectionDownFlags", 80, 8, 0x0204, 1724},
    {"HitDirectionStraightFlags", 88, 8, 0x0204, 1725},
    {"MeleeHitFlags", 96, 8, 0x0204, 1726},
    {"ProjectileHitFlags", 104, 8, 0x0204, 1727},
    {"ConcussionHitFlags", 112, 8, 0x0204, 1728},
    {"ImpactMagnitudeOverride", 120, 4, 0x0008, 6999},
    {"MinimumImpactAngle", 124, 4, 0x0008, 7000},
    {"TargetWeight", 128, 4, 0x0008, 7001},
    {"TargetFPS", 132, 4, 0x0008, 7002},
    {"BlendInComplete", 136, 2, 0x0008, 7003},
    {"EnableImpactDecay", 138, 1, 0x0014, 1658},
    {"TryOverrideImpactDirection", 139, 1, 0x0014, 1659},
};

inline constexpr Field kFields_02BF[] = {
    {"JointName", 24, 8, 0x0018, 0},
    {"PositionBlackboardVariableKey", 32, 8, 0x0010, 0},
    {"QuatBlackboardVariableKey", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_02C2[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"TwistWeight", 8, 4, 0x0008, 7010},
    {"SpringFrequency", 12, 4, 0x0008, 7011},
    {"SpringDampingRatio", 16, 4, 0x0008, 7012},
};

inline constexpr Field kFields_02C3[] = {
    {"ReferenceJoint", 0, 8, 0x0010, 0},
    {"ReferenceJointTwistAxis", 8, 1, 0x0104, 1741},
    {"TwistWeight", 12, 4, 0x0008, 7013},
    {"Joints", 16, 12, 0x0024, 404},
};

inline constexpr Field kFields_02C4[] = {
    {"PartFlags", 0, 8, 0x0204, 1742},
    {"HitFlags", 8, 8, 0x0204, 1743},
};

inline constexpr Field kFields_02C5[] = {
    {"NoTwistFlagCombos", 24, 12, 0x0024, 405},
    {"ProjectileTwistDeadzoneRadius", 36, 4, 0x0008, 7016},
    {"JointSegments", 40, 12, 0x0024, 406},
    {"TwistMagnitude", 52, 4, 0x0008, 7017},
    {"PartFlags", 56, 8, 0x0204, 1748},
    {"ConcussionHitFlags", 64, 8, 0x0204, 1749},
    {"ProjectileHitFlags", 72, 8, 0x0204, 1750},
    {"ProjectileForceUseAttackerDirHackFlags", 80, 8, 0x0204, 1751},
    {"MeleeHitFlags", 88, 8, 0x0204, 1752},
    {"HitDirectionLeftFlags", 96, 8, 0x0204, 1753},
    {"HitDirectionRightFlags", 104, 8, 0x0204, 1754},
    {"Enabled", 112, 1, 0x0014, 1660},
    {"TwistOnConcussion", 113, 1, 0x0014, 1661},
};

inline constexpr Field kFields_02C7[] = {
    {"MarkersGameObjectName", 24, 8, 0x0018, 430},
    {"Duration", 32, 4, 0x0008, 7022},
};

inline constexpr Field kFields_02C9[] = {
    {"ModulateTarget", 24, 1, 0x0104, 1771},
    {"TargetValue", 28, 4, 0x0008, 7028},
    {"ModulateVelocity", 32, 4, 0x0008, 7029},
};

inline constexpr Field kFields_02CA[] = {
    {"POIName", 24, 8, 0x0018, 0},
    {"ExactPos", 32, 0, 0x002C, 6},
    {"ArgVal", 40, 4, 0x0008, 7035},
    {"YRotation", 44, 4, 0x0008, 7036},
};

inline constexpr Field kFields_02CB[] = {
    {"ID", 24, 8, 0x0010, 0},
    {"Radius", 32, 4, 0x0008, 7039},
    {"YRotation", 36, 4, 0x0008, 7040},
    {"YHeight", 40, 4, 0x0008, 7041},
};

inline constexpr Field kFields_02CC[] = {
    {"Impulse", 24, 0, 0x002C, 6},
    {"Duration", 32, 4, 0x0008, 7047},
    {"FaceOverride", 36, 1, 0x0104, 1784},
    {"AccumulationMode", 37, 1, 0x0104, 1785},
    {"IgnoreNoImpulse", 38, 1, 0x0014, 1662},
};

inline constexpr Field kFields_02CD[] = {
    {"MaxFallDistance", 24, 4, 0x0008, 7050},
};

inline constexpr Field kFields_02CE[] = {
    {"InterestingTargetRedirectionAngle", 24, 4, 0x0008, 7053},
    {"InterestingTargetRedirectionMaximumDistance", 28, 4, 0x0008, 7054},
    {"InterestingTargetRedirectionMinimumDistance", 32, 4, 0x0008, 7055},
};

inline constexpr Field kFields_02D0[] = {
    {"Radius", 24, 4, 0x0008, 7060},
    {"Height", 28, 4, 0x0008, 7061},
};

inline constexpr Field kFields_02D1[] = {
    {"ExcludeFilters", 24, 12, 0x0024, 407},
    {"SnapDistance", 36, 4, 0x0008, 7064},
    {"JointName", 40, 8, 0x0010, 0},
    {"AlignJointName", 48, 8, 0x0010, 0},
    {"BoxDimension", 56, 0, 0x002C, 6},
    {"OffsetPosition", 62, 0, 0x002C, 6},
    {"OffsetPushCenter", 68, 0, 0x002C, 6},
    {"PushLineEnd", 74, 0, 0x002C, 6},
    {"StartScale", 80, 0, 0x002C, 6},
    {"ClearMode", 86, 1, 0x0104, 1806},
    {"ClearShape", 87, 1, 0x0104, 1807},
    {"MinForceDistance", 88, 4, 0x0008, 7080},
    {"PushOutRadius", 92, 4, 0x0008, 7081},
    {"SpeedCenter", 96, 4, 0x0008, 7082},
    {"SpeedEdge", 100, 4, 0x0008, 7083},
    {"Radius", 104, 4, 0x0008, 7084},
    {"CylinderLength", 108, 4, 0x0008, 7085},
    {"ScaleInTime", 112, 4, 0x0008, 7086},
    {"ScaleOutTime", 116, 4, 0x0008, 7087},
    {"ArgVal", 120, 4, 0x0008, 7088},
    {"ClearFlags", 124, 1, 0x0204, 1808},
    {"AffectType", 125, 1, 0x0104, 1809},
    {"ExcludeDeadAndDying", 126, 1, 0x0014, 1663},
    {"MaxNumSnapped", 127, 1, 0x0000, 723},
    {"LockAxis", 128, 1, 0x0014, 1664},
};

inline constexpr Field kFields_02D2[] = {
    {"ArgData", 24, 8, 0x001C, 65535},
    {"ArgInt", 32, 4, 0x0000, 725},
    {"StateOverride", 36, 2, 0x0104, 1814},
    {"Action", 38, 1, 0x0104, 1815},
    {"CommonFlags", 39, 1, 0x0204, 1816},
};

inline constexpr Field kFields_02D3[] = {
    {"Field1", 24, 4, 0x0000, 727},
    {"Field2", 28, 4, 0x0000, 728},
    {"SendToUI", 32, 1, 0x0000, 729},
    {"LocalOnly", 33, 1, 0x0000, 730},
};

inline constexpr Field kFields_02D4[] = {
    {"BranchName", 24, 8, 0x0010, 0},
    {"Duration", 32, 4, 0x0008, 7095},
    {"Timer", 36, 1, 0x0000, 732},
    {"CancelCurrent", 37, 1, 0x0000, 733},
};

inline constexpr Field kFields_02D5[] = {
    {"Timer", 24, 1, 0x0000, 735},
};

inline constexpr Field kFields_02D6[] = {
    {"HeroJoint", 24, 8, 0x0010, 447},
    {"Joint", 32, 8, 0x0010, 448},
};

inline constexpr Field kFields_02D7[] = {
    {"Reason", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_02D8[] = {
    {"Reason", 24, 8, 0x0010, 0},
    {"Immediate", 32, 1, 0x0014, 1665},
    {"Interruptable", 33, 1, 0x0014, 1666},
};

inline constexpr Field kFields_02DA[] = {
    {"SourceJoint", 24, 8, 0x0010, 452},
    {"TargetJoint", 32, 8, 0x0010, 453},
    {"AlignMode", 40, 1, 0x0104, 1849},
};

inline constexpr Field kFields_02DC[] = {
    {"ID", 48, 8, 0x0010, 0},
    {"Radius", 56, 4, 0x0008, 7112},
};

inline constexpr Field kFields_02DD[] = {
    {"MaterialAnimName", 24, 8, 0x0010, 0},
    {"ExitAnimName", 32, 8, 0x0010, 0},
    {"TargetObjectName", 40, 8, 0x0010, 0},
    {"IdleTime", 48, 4, 0x0008, 7115},
    {"TriggerExitOnMoveEnd", 52, 1, 0x0014, 1667},
    {"MaterialAnimType", 53, 1, 0x0104, 1864},
    {"TargetObjectType", 54, 1, 0x0104, 1865},
};

inline constexpr Field kFields_02E0[] = {
    {"BroadcastContext", 24, 8, 0x0010, 0},
    {"FilterData", 32, 12, 0x0024, 408},
    {"FilterFlags", 44, 1, 0x0204, 1878},
    {"ReactionFlags", 45, 1, 0x0204, 1879},
    {"MaxReactions", 46, 1, 0x0000, 747},
};

inline constexpr Field kFields_02E1[] = {
    {"EventName", 24, 8, 0x0010, 0},
    {"Target", 32, 1, 0x0104, 1884},
    {"EnemyID", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_02E2[] = {
    {"LeanFactor", 24, 4, 0x0008, 7126},
    {"Damping", 28, 4, 0x0008, 7127},
};

inline constexpr Field kFields_02E4[] = {
    {"CreatureName", 24, 8, 0x0018, 0},
    {"InitialBranch", 32, 8, 0x0010, 0},
    {"SpawnJoint", 40, 8, 0x0010, 468},
    {"Heap", 48, 4, 0x0000, 752},
};

inline constexpr Field kFields_02E5[] = {
    {"PlayFXList", 24, 8, 0x001C, 273},
    {"EffectSourceJoint", 32, 8, 0x0010, 470},
    {"EffectTargetJoint", 40, 8, 0x0010, 471},
    {"TargetJointID", 48, 1, 0x0000, 754},
};

inline constexpr Field kFields_02E6[] = {
    {"AttachmentModeFlags", 24, 4, 0x0204, 1905},
    {"TargetJoint", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_02E7[] = {
    {"Amplitude", 24, 4, 0x0008, 7138},
    {"FadeOutTime", 28, 4, 0x0008, 7139},
    {"ForceDuration", 32, 4, 0x0008, 7140},
    {"PatternCycleTime", 36, 4, 0x0008, 7141},
    {"Motor", 40, 1, 0x0204, 1910},
    {"PatternType", 41, 1, 0x0104, 1911},
    {"EnvelopeType", 42, 1, 0x0104, 1912},
    {"EnableOnPS5", 43, 1, 0x0014, 1668},
};

inline constexpr Field kFields_02E8[] = {
    {"EnableList", 24, 12, 0x0024, 409},
    {"HideList", 40, 12, 0x0024, 410},
    {"ShowList", 56, 12, 0x0024, 411},
    {"TrailList", 72, 12, 0x0024, 412},
    {"TrailHideList", 88, 12, 0x0024, 413},
    {"ChainGlowList", 104, 12, 0x0024, 414},
    {"ChainHideList", 120, 12, 0x0024, 415},
};

inline constexpr Field kFields_02E9[] = {
    {"WeaponType", 24, 4, 0x0000, 759},
    {"AttachmentName", 32, 8, 0x0010, 0},
    {"AttachMode", 40, 8, 0x0010, 0},
    {"SwitchPercentage", 48, 4, 0x0008, 7146},
    {"StartScaleWeapon", 52, 4, 0x0008, 7147},
    {"StopScaleWeapon", 56, 4, 0x0008, 7148},
    {"ApplyFlagsOnStart", 60, 1, 0x0014, 1669},
    {"ForceScaleOnExit", 61, 1, 0x0014, 1670},
};

inline constexpr Field kFields_02EA[] = {
    {"WeaponType", 0, 4, 0x0000, 760},
    {"SwitchMoveType", 4, 1, 0x0104, 1928},
    {"SwitchPercentage", 6, 2, 0x0008, 7149},
    {"StartScaleWeapon", 8, 2, 0x0008, 7150},
    {"StopScaleWeapon", 10, 2, 0x0008, 7151},
};

inline constexpr Field kFields_02EB[] = {
    {"SwitchWeapon", 24, 8, 0x001C, 746},
};

inline constexpr Field kFields_02EC[] = {
    {"SwitchWeapon", 24, 8, 0x001C, 746},
};

inline constexpr Field kFields_02ED[] = {
    {"WeaponType", 24, 4, 0x0000, 764},
    {"Scale", 28, 4, 0x0008, 7158},
    {"BlendInEnd", 32, 2, 0x0008, 7159},
    {"BlendOutStart", 34, 2, 0x0008, 7160},
};

inline constexpr Field kFields_02EE[] = {
    {"ActiveJointName", 24, 8, 0x0010, 0},
    {"StowJointName", 32, 8, 0x0010, 0},
    {"WeaponType", 40, 4, 0x0000, 766},
    {"Persist", 44, 1, 0x0014, 1671},
};

inline constexpr Field kFields_02F0[] = {
    {"WeaponType", 24, 4, 0x0000, 769},
};

inline constexpr Field kFields_02F1[] = {
    {"WeaponType", 24, 4, 0x0000, 771},
    {"Move", 32, 8, 0x001C, 1283},
};

inline constexpr Field kFields_02F2[] = {
    {"WeaponType", 24, 4, 0x0000, 773},
    {"PauseDefaultTimer", 28, 1, 0x0014, 1672},
    {"Persist", 29, 1, 0x0014, 1673},
};

inline constexpr Field kFields_02F3[] = {
    {"WeaponType", 24, 4, 0x0000, 775},
};

inline constexpr Field kFields_02F4[] = {
    {"NonCreatureDetonationTiming", 24, 0, 0x002C, 1518},
    {"CreatureDetonationTiming", 44, 0, 0x002C, 1518},
    {"EnvironmentConcussionList", 64, 12, 0x0024, 416},
    {"WeaponType", 76, 4, 0x0000, 777},
    {"CreatureConcussionList", 80, 12, 0x0024, 417},
    {"RadialBlastOutSpeed", 92, 4, 0x0008, 7185},
    {"JointName", 96, 8, 0x0010, 0},
    {"RadialBlastAcceleration", 104, 4, 0x0008, 7186},
    {"RadialBlastRandomOffset", 108, 4, 0x0008, 7187},
    {"DetonatePattern", 112, 1, 0x0104, 1969},
    {"CreatureDetonatePattern", 113, 1, 0x0104, 1970},
    {"PrioritizeFauxEmbedded", 114, 1, 0x0014, 1674},
    {"TriggerVScriptEvent", 115, 1, 0x0014, 1675},
    {"SpawnShrapnelCount", 116, 1, 0x0000, 778},
};

inline constexpr Field kFields_02F5[] = {
    {"WeaponType", 24, 4, 0x0000, 780},
    {"MaxSecondsSinceHit", 28, 4, 0x0008, 7190},
    {"EmbedTime", 32, 2, 0x0008, 7191},
    {"PenetrationOffsetAlongWeaponAxis", 36, 4, 0x0008, 7192},
    {"HitToleranceRadius", 40, 4, 0x0008, 7193},
    {"EmbedFlags", 44, 1, 0x0204, 1975},
};

inline constexpr Field kFields_02F6[] = {
    {"HandJointName", 24, 8, 0x0010, 488},
    {"WeaponType", 32, 4, 0x0000, 782},
    {"MaxSecondsSinceHit", 36, 4, 0x0008, 7196},
    {"AdjustDuration", 40, 4, 0x0008, 7197},
    {"EmbedTime", 44, 2, 0x0008, 7198},
    {"EmbedFlags", 46, 1, 0x0204, 1980},
};

inline constexpr Field kFields_02F7[] = {
    {"EmbedJointName", 24, 8, 0x0010, 0},
    {"TransitionJointName", 32, 8, 0x0010, 0},
    {"WeaponType", 40, 4, 0x0000, 784},
    {"TransitionStartTime", 44, 2, 0x0008, 7201},
};

inline constexpr Field kFields_02F8[] = {
    {"WeaponType", 24, 4, 0x0000, 786},
    {"EmbedJointName", 32, 8, 0x0010, 0},
    {"DepthOffset", 40, 4, 0x0008, 7204},
    {"DepthBlendStart", 44, 2, 0x0008, 7205},
    {"DepthBlendEnd", 46, 2, 0x0008, 7206},
};

inline constexpr Field kFields_02F9[] = {
    {"EmbedJointName", 24, 8, 0x0010, 0},
    {"Position", 32, 0, 0x002C, 6},
    {"Rotation", 38, 0, 0x002C, 6},
    {"PositionNoise", 44, 0, 0x002C, 6},
    {"RotationNoise", 50, 0, 0x002C, 6},
    {"WeaponType", 56, 4, 0x0000, 788},
    {"EmbedTime", 60, 2, 0x0008, 7221},
};

inline constexpr Field kFields_02FA[] = {
    {"WeaponType", 24, 4, 0x0000, 790},
};

inline constexpr Field kFields_02FB[] = {
    {"SnapFlags", 24, 1, 0x0204, 2001},
};

inline constexpr Field kFields_02FC[] = {
    {"SubObjectName", 0, 8, 0x0010, 0},
    {"JointName", 8, 8, 0x0010, 0},
    {"UseTarget", 16, 1, 0x0014, 1676},
};

inline constexpr Field kFields_02FD[] = {
    {"JointNames", 0, 12, 0x0024, 418},
    {"IsTargeting", 12, 1, 0x0014, 1677},
};

inline constexpr Field kFields_02FE[] = {
    {"ParentJoint", 0, 0, 0x002C, 764},
    {"ChildJoint", 24, 0, 0x002C, 764},
    {"JointOffset", 48, 0, 0x002C, 6},
    {"BowString", 56, 8, 0x001C, 765},
    {"TweenInTime", 64, 4, 0x0008, 7229},
    {"TweenOutTime", 68, 4, 0x0008, 7230},
};

inline constexpr Field kFields_02FF[] = {
    {"SnapData", 24, 8, 0x001C, 766},
};

inline constexpr Field kFields_0300[] = {
    {"AttachName", 24, 8, 0x0010, 0},
    {"WeaponType", 32, 4, 0x0000, 794},
    {"ThrowMode", 40, 8, 0x001C, 423},
};

inline constexpr Field kFields_0301[] = {
    {"AttachName", 24, 8, 0x0010, 0},
    {"WeaponType", 32, 12, 0x0024, 419},
    {"DropMode", 44, 1, 0x0104, 2014},
    {"EmbeddedInMe", 45, 1, 0x0014, 1680},
    {"MaxDrops", 48, 4, 0x0000, 797},
    {"HoldTime", 52, 4, 0x0008, 7237},
};

inline constexpr Field kFields_0302[] = {
    {"WeaponType", 24, 4, 0x0000, 799},
    {"AttachName", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_0303[] = {
    {"WeaponMode", 24, 8, 0x0010, 0},
    {"Immediate", 32, 1, 0x0014, 1681},
    {"Interruptable", 33, 1, 0x0014, 1682},
};

inline constexpr Field kFields_0307[] = {
    {"AttachName", 24, 8, 0x0010, 0},
    {"WeaponType", 32, 4, 0x0000, 805},
    {"PinAction", 36, 1, 0x0104, 2039},
};

inline constexpr Field kFields_0308[] = {
    {"CaughtAction", 24, 1, 0x0104, 2044},
    {"MaxActions", 28, 4, 0x0000, 807},
};

inline constexpr Field kFields_0309[] = {
    {"AttachName", 24, 8, 0x0010, 0},
    {"Group", 32, 8, 0x0010, 0},
    {"WeaponType", 40, 4, 0x0000, 809},
    {"SpeedXZ", 44, 4, 0x0008, 7254},
    {"ScaleGravity", 48, 4, 0x0008, 7255},
    {"MinRange", 52, 4, 0x0008, 7256},
    {"MaxRange", 56, 4, 0x0008, 7257},
    {"TimeMaxRange", 60, 4, 0x0008, 7258},
    {"ReticleFlags", 64, 1, 0x0204, 2049},
};

inline constexpr Field kFields_030A[] = {
    {"WeaponType", 24, 4, 0x0000, 811},
    {"BaseWeaponData", 32, 8, 0x001C, 423},
    {"JointName", 40, 8, 0x0010, 0},
    {"FramesToSimulate", 48, 4, 0x0000, 812},
    {"StartOffset", 52, 0, 0x002C, 6},
    {"AngleBetweenTwoPoints", 60, 4, 0x0008, 7264},
};

inline constexpr Field kFields_030B[] = {
    {"Side", 24, 4, 0x0000, 814},
    {"GlowFlags", 28, 1, 0x0204, 2058},
    {"RampAlong", 32, 8, 0x001C, 287},
    {"RampMax", 40, 8, 0x001C, 287},
    {"GlowEndTime", 48, 4, 0x0008, 7267},
    {"EmissiveMin", 52, 4, 0x0008, 7268},
    {"EmissiveMaxBegin", 56, 4, 0x0008, 7269},
    {"EmissiveMaxEnd", 60, 4, 0x0008, 7270},
    {"PulseWidth", 64, 4, 0x0008, 7271},
    {"GlowChainEffect", 72, 8, 0x001C, 272},
    {"GlowEffectTint", 80, 0, 0x002C, 2},
};

inline constexpr Field kFields_030C[] = {
    {"RampAlong", 24, 8, 0x001C, 287},
    {"Tint", 32, 0, 0x002C, 2},
    {"ChainFX", 48, 8, 0x001C, 272},
    {"Side", 56, 4, 0x0000, 816},
    {"MinSeparation", 60, 4, 0x0008, 7282},
    {"FinalTime", 64, 4, 0x0008, 7283},
    {"ScaleMin", 68, 4, 0x0008, 7284},
    {"ScaleMax", 72, 4, 0x0008, 7285},
    {"ChainFXFlags", 76, 1, 0x0204, 2063},
    {"FromBlade", 77, 1, 0x0014, 1683},
};

inline constexpr Field kFields_030D[] = {
    {"Side", 24, 4, 0x0000, 818},
    {"RotateAroundZ", 28, 4, 0x0008, 7288},
    {"Amplitude", 32, 4, 0x0008, 7289},
    {"Wavelength", 36, 4, 0x0008, 7290},
    {"Speed", 40, 4, 0x0008, 7291},
    {"DecayTime", 44, 4, 0x0008, 7292},
};

inline constexpr Field kFields_030E[] = {
    {"ChainDriveList", 24, 12, 0x0024, 420},
    {"Side", 36, 4, 0x0000, 820},
};

inline constexpr Field kFields_030F[] = {
    {"Side", 24, 4, 0x0000, 822},
    {"ScaleGravity", 28, 4, 0x0008, 7297},
    {"EaseIn", 32, 4, 0x0008, 7298},
    {"ExtraSlack", 36, 4, 0x0008, 7299},
};

inline constexpr Field kFields_0310[] = {
    {"WeaponTrailData", 24, 8, 0x001C, 237},
};

inline constexpr Field kFields_0311[] = {
    {"WeaponType", 24, 4, 0x0000, 825},
    {"TrailType", 32, 8, 0x0010, 513},
};

inline constexpr Field kFields_0312[] = {
    {"Side", 32, 4, 0x0000, 827},
};

inline constexpr Field kFields_0313[] = {
    {"Side", 24, 4, 0x0000, 829},
    {"EnableCollision", 28, 1, 0x0000, 830},
    {"AllowAnimBlending", 29, 1, 0x0000, 831},
    {"ChainProperties", 32, 0, 0x002C, 442},
    {"HitProperties", 64, 8, 0x001C, 443},
};

inline constexpr Field kFields_0314[] = {
    {"Side", 24, 4, 0x0000, 833},
    {"EnableCollision", 28, 1, 0x0000, 834},
};

inline constexpr Field kFields_0315[] = {
    {"Side", 24, 4, 0x0000, 836},
    {"ChainDamping", 28, 4, 0x0008, 7317},
    {"ChainDampingY", 32, 4, 0x0008, 7318},
    {"GravityFactor", 36, 4, 0x0008, 7319},
};

inline constexpr Field kFields_0316[] = {
    {"Side", 24, 4, 0x0000, 838},
    {"SlackLength", 28, 4, 0x0008, 7322},
    {"SlackRecoilSpeed", 32, 4, 0x0008, 7323},
    {"BlendTime", 36, 4, 0x0008, 7324},
};

inline constexpr Field kFields_0317[] = {
    {"Side", 24, 4, 0x0000, 840},
    {"SlackLength", 28, 4, 0x0008, 7327},
    {"SlackRecoilSpeed", 32, 4, 0x0008, 7328},
    {"BlendTime", 36, 4, 0x0008, 7329},
    {"BlendDelay", 40, 4, 0x0008, 7330},
};

inline constexpr Field kFields_0318[] = {
    {"Side", 24, 4, 0x0000, 842},
    {"TimeToStop", 28, 2, 0x0008, 7333},
};

inline constexpr Field kFields_0319[] = {
    {"Side", 24, 4, 0x0000, 844},
    {"FrameDuration", 28, 4, 0x0000, 845},
    {"NumLoops", 32, 4, 0x0000, 846},
    {"MaxRadius", 36, 4, 0x0008, 7336},
    {"EnvelopeMinScale", 40, 4, 0x0008, 7337},
    {"EnvelopeMaxScale", 44, 4, 0x0008, 7338},
    {"HorizontalScale", 48, 4, 0x0008, 7339},
    {"VerticalScale", 52, 4, 0x0008, 7340},
};

inline constexpr Field kFields_031A[] = {
    {"Side", 24, 4, 0x0000, 848},
};

inline constexpr Field kFields_031B[] = {
    {"Side", 24, 4, 0x0000, 850},
};

inline constexpr Field kFields_031C[] = {
    {"Side", 24, 4, 0x0000, 852},
    {"TautCurveAmount", 28, 4, 0x0008, 7347},
    {"TautNoise", 32, 4, 0x0008, 7348},
    {"TautStartForwardBias", 36, 4, 0x0008, 7349},
    {"TautStartSideBias", 40, 4, 0x0008, 7350},
    {"TautEndForwardBias", 44, 4, 0x0008, 7351},
    {"TautEndSideBias", 48, 4, 0x0008, 7352},
    {"OverrideTautCurveAmount", 52, 1, 0x0014, 1684},
    {"OverrideTautNoise", 53, 1, 0x0014, 1685},
    {"OverrideTautStartForwardBias", 54, 1, 0x0014, 1686},
    {"OverrideTautStartSideBias", 55, 1, 0x0014, 1687},
    {"OverrideTautEndForwardBias", 56, 1, 0x0014, 1688},
    {"OverrideTautEndSideBias", 57, 1, 0x0014, 1689},
};

inline constexpr Field kFields_031D[] = {
    {"Side", 24, 4, 0x0000, 854},
    {"TautLengthOverride", 28, 4, 0x0008, 7355},
};

inline constexpr Field kFields_031E[] = {
    {"Side", 24, 4, 0x0000, 856},
};

inline constexpr Field kFields_031F[] = {
    {"Side", 24, 4, 0x0000, 858},
};

inline constexpr Field kFields_0320[] = {
    {"Side", 24, 4, 0x0000, 860},
    {"BlendTime", 28, 4, 0x0008, 7362},
};

inline constexpr Field kFields_0321[] = {
    {"Side", 24, 4, 0x0000, 862},
};

inline constexpr Field kFields_0322[] = {
    {"Side", 24, 4, 0x0000, 864},
    {"GrabJointName", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_0323[] = {
    {"GrabJointName", 24, 8, 0x0010, 0},
    {"Side", 32, 4, 0x0000, 866},
    {"GrabbedChainPercentage", 36, 4, 0x0008, 7369},
};

inline constexpr Field kFields_0324[] = {
    {"Range", 24, 4, 0x0008, 7372},
    {"TargetingFlags", 28, 2, 0x0204, 2160},
    {"TargetWeightID", 30, 1, 0x0000, 868},
    {"TargetJointID", 31, 1, 0x0000, 869},
};

inline constexpr Field kFields_0325[] = {
    {"TargetWeight", 24, 4, 0x0008, 7375},
    {"TargetWeightID", 28, 1, 0x0000, 871},
};

inline constexpr Field kFields_0327[] = {
    {"Reacquire", 24, 1, 0x0014, 1690},
    {"OffScreen", 25, 1, 0x0014, 1691},
    {"Invisible", 26, 1, 0x0014, 1692},
};

inline constexpr Field kFields_0329[] = {
    {"Name", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_032A[] = {
    {"Name", 24, 8, 0x0010, 0},
    {"TweenOut", 32, 4, 0x0008, 7386},
};

inline constexpr Field kFields_032C[] = {
    {"CharacterID", 24, 8, 0x0010, 0},
    {"SynchID", 32, 1, 0x0000, 879},
    {"UsePrimaryCompanion", 33, 1, 0x0014, 1693},
    {"BlackboardType", 34, 1, 0x0104, 2193},
    {"BlackboardVarName", 40, 8, 0x0010, 0},
    {"MoveOverride", 48, 8, 0x0010, 0},
};

inline constexpr Field kFields_032D[] = {
    {"MovePairs", 24, 12, 0x0024, 421},
    {"MoveToPlayWhenNoMatch", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_032E[] = {
    {"ObjectName", 24, 8, 0x0010, 0},
    {"AnimationName", 32, 8, 0x0010, 0},
    {"SlaveID", 40, 1, 0x0000, 882},
};

inline constexpr Field kFields_032F[] = {
    {"ObjectName", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0330[] = {
    {"CreatureID", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0331[] = {
    {"CreatureID", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0332[] = {
    {"CharacterID", 24, 8, 0x0010, 0},
    {"SynchID", 32, 1, 0x0000, 887},
};

inline constexpr Field kFields_0333[] = {
    {"CharacterID", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0334[] = {
    {"EndPlaybackModifierPhase", 24, 4, 0x0008, 7407},
    {"BasePlaybackSpeed", 28, 4, 0x0008, 7408},
};

inline constexpr Field kFields_0337[] = {
    {"StartAlpha", 24, 4, 0x0008, 7415},
    {"EndAlpha", 28, 4, 0x0008, 7416},
    {"EarlyEndAlpha", 32, 4, 0x0008, 7417},
    {"ApplyToWeapons", 36, 1, 0x0014, 1694},
    {"ApplyToEmbeds", 37, 1, 0x0014, 1695},
};

inline constexpr Field kFields_0338[] = {
    {"Trophy", 24, 4, 0x0000, 894},
    {"Threshold", 28, 4, 0x0000, 895},
};

inline constexpr Field kFields_0339[] = {
    {"Trophy", 24, 4, 0x0000, 897},
    {"FlagNumber", 28, 4, 0x0000, 898},
    {"MaxFlags", 32, 4, 0x0000, 899},
};

inline constexpr Field kFields_033A[] = {
    {"Attribute", 24, 8, 0x0010, 0},
    {"StatsType", 32, 8, 0x0010, 0},
    {"AddBuff", 40, 4, 0x0008, 7424},
    {"ApplyTo", 44, 1, 0x0104, 2250},
    {"Range", 48, 4, 0x0008, 7425},
    {"Duration", 52, 4, 0x0008, 7426},
};

inline constexpr Field kFields_033D[] = {
    {"ArgData", 24, 8, 0x001C, 324},
};

inline constexpr Field kFields_033E[] = {
    {"ArgData", 24, 8, 0x001C, 325},
};

inline constexpr Field kFields_0341[] = {
    {"Distance", 24, 4, 0x0008, 7441},
    {"Size", 28, 4, 0x0008, 7442},
};

inline constexpr Field kFields_0342[] = {
    {"ArgVal", 24, 4, 0x0008, 7445},
    {"SendTo", 28, 1, 0x0104, 2283},
};

inline constexpr Field kFields_0343[] = {
    {"GOName", 24, 8, 0x0010, 0},
    {"JointName", 32, 8, 0x0010, 0},
    {"Heap", 40, 4, 0x0000, 910},
    {"MinRadius", 44, 4, 0x0008, 7448},
    {"MaxRadius", 48, 4, 0x0008, 7449},
    {"Button", 52, 1, 0x0104, 2288},
    {"ContractionRate", 56, 4, 0x0008, 7450},
    {"ExpandAmount", 60, 4, 0x0008, 7451},
};

inline constexpr Field kFields_0344[] = {
    {"ArgData", 24, 8, 0x001C, 268},
    {"Visibility", 32, 1, 0x0204, 2293},
};

inline constexpr Field kFields_0345[] = {
    {"ArgData", 24, 8, 0x001C, 272},
    {"Tint", 32, 0, 0x002C, 2},
};

inline constexpr Field kFields_0346[] = {
    {"MFXTrigger", 24, 8, 0x001C, 263},
};

inline constexpr Field kFields_0347[] = {
    {"DecayFX", 24, 8, 0x001C, 265},
};

inline constexpr Field kFields_0348[] = {
    {"Radius", 24, 4, 0x0008, 7466},
    {"TimeScale", 28, 4, 0x0008, 7467},
};

inline constexpr Field kFields_034A[] = {
    {"Name", 24, 8, 0x0018, 0},
    {"Enable", 32, 1, 0x0000, 918},
};

inline constexpr Field kFields_034B[] = {
    {"StartProgression", 24, 4, 0x0008, 7474},
    {"EndProgression", 28, 4, 0x0008, 7475},
    {"TransferType", 32, 1, 0x0104, 2322},
};

inline constexpr Field kFields_034C[] = {
    {"StartProgression", 24, 4, 0x0008, 7478},
    {"EndProgression", 28, 4, 0x0008, 7479},
    {"StartOrient", 32, 1, 0x0104, 2327},
    {"EndOrient", 33, 1, 0x0104, 2328},
};

inline constexpr Field kFields_0350[] = {
    {"JointName", 24, 8, 0x0010, 0},
    {"Enable", 32, 1, 0x0000, 925},
};

inline constexpr Field kFields_0351[] = {
    {"FromJoint", 0, 8, 0x0010, 0},
    {"ToJoint", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0352[] = {
    {"DefaultTint", 0, 0, 0x002C, 1},
    {"TintBhvr", 16, 1, 0x0104, 2345},
    {"LineGOName", 24, 8, 0x0010, 0},
    {"Lines", 32, 12, 0x0024, 422},
};

inline constexpr Field kFields_0353[] = {
    {"LineSet", 24, 8, 0x001C, 850},
};

inline constexpr Field kFields_0354[] = {
    {"TimeScale", 24, 4, 0x0008, 7496},
    {"ForEver", 28, 1, 0x0000, 928},
};

inline constexpr Field kFields_0356[] = {
    {"Enabled", 24, 1, 0x0000, 931},
};

inline constexpr Field kFields_0357[] = {
    {"PinMode", 24, 1, 0x0104, 2366},
    {"SpeedThreshold", 28, 4, 0x0008, 7503},
    {"Duration", 32, 4, 0x0008, 7504},
    {"ExtraSteps", 36, 1, 0x0000, 933},
};

inline constexpr Field kFields_0358[] = {
    {"Enabled", 24, 1, 0x0000, 935},
};

inline constexpr Field kFields_0359[] = {
    {"Persist", 24, 1, 0x0000, 937},
    {"Reset", 25, 1, 0x0000, 938},
    {"Color0", 32, 0, 0x002C, 1},
};

inline constexpr Field kFields_035A[] = {
    {"Duration", 24, 4, 0x0008, 7515},
};

inline constexpr Field kFields_035B[] = {
    {"HideMesh", 24, 1, 0x0000, 941},
};

inline constexpr Field kFields_035D[] = {
    {"JointName", 24, 8, 0x0010, 588},
    {"HasTargetTint", 32, 0, 0x002C, 1},
    {"HasNoTargetTint", 48, 0, 0x002C, 1},
    {"LineGOName", 64, 8, 0x0010, 0},
    {"TrailGOName", 72, 8, 0x0010, 0},
    {"ProtractorLineOrigJointName", 80, 8, 0x0010, 589},
    {"ProtractorLineStartJointName", 88, 8, 0x0010, 590},
    {"ProtractorLineEndJointName", 96, 8, 0x0010, 591},
    {"ReturnDuration", 104, 4, 0x0008, 7530},
    {"FadeOutStartTime", 108, 4, 0x0008, 7531},
    {"FadeOutDuration", 112, 4, 0x0008, 7532},
    {"MinTargetDistance", 116, 4, 0x0008, 7533},
    {"MaxTargetDistance", 120, 4, 0x0008, 7534},
    {"MaxAngle", 124, 4, 0x0008, 7535},
    {"LineExtendDuration", 128, 4, 0x0008, 7536},
    {"LineStayDuration", 132, 4, 0x0008, 7537},
    {"LineShortenDuration", 136, 4, 0x0008, 7538},
    {"ProtractorLineRetractStart", 140, 4, 0x0008, 7539},
    {"ProtractorLineRetractDuration", 144, 4, 0x0008, 7540},
    {"MeleeLineFlags", 148, 1, 0x0204, 2391},
    {"TargetJointID", 149, 1, 0x0000, 944},
    {"TargetableTypes", 150, 1, 0x0204, 2392},
};

inline constexpr Field kFields_035E[] = {
    {"State", 24, 1, 0x0104, 2397},
    {"MotorActivationCriteria", 25, 1, 0x0104, 2398},
    {"VelocityFromAnimation", 28, 4, 0x0008, 7543},
    {"RandomActivateWindowEnd", 32, 4, 0x0008, 7544},
    {"GroundCollisionDetectionStart", 36, 4, 0x0008, 7545},
    {"GroundCollisionDetectionStop", 40, 4, 0x0008, 7546},
    {"WallCollisionDetectionStart", 44, 4, 0x0008, 7547},
    {"WallCollisionDetectionStop", 48, 4, 0x0008, 7548},
    {"CreatureCollisionDetectionStart", 52, 4, 0x0008, 7549},
    {"CreatureCollisionDetectionStop", 56, 4, 0x0008, 7550},
    {"ForceRagdollTime", 60, 4, 0x0008, 7551},
    {"AllowImpulseStart", 64, 4, 0x0008, 7552},
    {"Gravity", 68, 4, 0x0008, 7553},
    {"TimeUntilFullRagdoll", 72, 4, 0x0008, 7554},
    {"PassiveJointFriction", 76, 4, 0x0008, 7555},
    {"UseAlternativeParams", 80, 8, 0x0010, 0},
    {"UseAlternativeParamsOnExit", 88, 8, 0x0010, 0},
    {"TimeToInhibitGroundPush", 96, 4, 0x0008, 7556},
    {"TimeToInhibitWallPush", 100, 4, 0x0008, 7557},
    {"TimeToInhibitRoll", 104, 4, 0x0008, 7558},
    {"UseCustomGravity", 108, 1, 0x0014, 1696},
    {"ActivateOnFlatGroundCollision", 109, 1, 0x0014, 1697},
    {"ActivateOnSlopedGroundCollision", 110, 1, 0x0014, 1698},
    {"ActivateOnWallCollision", 111, 1, 0x0014, 1699},
    {"ActivateOnRegularCreatureCollision", 112, 1, 0x0014, 1700},
    {"ActivateOnLargeCreatureCollision", 113, 1, 0x0014, 1701},
    {"IncludeCapsuleForActivateOnCollision", 114, 1, 0x0014, 1702},
};

inline constexpr Field kFields_035F[] = {
    {"DecapParts", 24, 2, 0x0204, 2403},
    {"AnimationVelocityScale", 28, 4, 0x0008, 7561},
    {"LinearImpulseMultiplier", 32, 4, 0x0008, 7562},
    {"AngularImpulseMultiplier", 36, 4, 0x0008, 7563},
    {"FallbackAngularSpeed", 40, 4, 0x0008, 7564},
    {"Domain_VerticalVelocityMin", 44, 4, 0x0008, 7565},
    {"Domain_VerticalVelocityMax", 48, 4, 0x0008, 7566},
    {"Range_VerticalVelocityMin", 52, 4, 0x0008, 7567},
    {"Range_VerticalVelocityMax", 56, 4, 0x0008, 7568},
    {"Domain_HorizontalVelocityMin", 60, 4, 0x0008, 7569},
    {"Domain_HorizontalVelocityMax", 64, 4, 0x0008, 7570},
    {"Range_HorizontalVelocityMin", 68, 4, 0x0008, 7571},
    {"Range_HorizontalVelocityMax", 72, 4, 0x0008, 7572},
    {"MaximumAngularSpeed", 76, 4, 0x0008, 7573},
};

inline constexpr Field kFields_0360[] = {
    {"FadeInEnd", 24, 4, 0x0008, 7576},
    {"FadeOutStart", 28, 4, 0x0008, 7577},
    {"EnableFootPinning", 32, 1, 0x0014, 1703},
    {"EnablePelvisAdjustment", 33, 1, 0x0014, 1704},
};

inline constexpr Field kFields_0363[] = {
    {"TargetJoint", 24, 8, 0x0010, 0},
    {"WeaponName", 32, 8, 0x0010, 598},
    {"LocalOffset", 40, 0, 0x002C, 6},
    {"Hand", 46, 1, 0x0104, 2420},
    {"Duration", 48, 4, 0x0008, 7587},
};

inline constexpr Field kFields_0364[] = {
    {"Hand", 24, 1, 0x0104, 2425},
};

inline constexpr Field kFields_0366[] = {
    {"Probe", 24, 0, 0x002C, 244},
};

inline constexpr Field kFields_0367[] = {
    {"BlendTime", 24, 4, 0x0008, 7599},
    {"MaxSpeed", 28, 4, 0x0008, 7600},
    {"DebugAutoSlowMo", 32, 4, 0x0008, 7601},
    {"ProbeLength", 36, 4, 0x0008, 7602},
    {"ProbeHeight", 40, 4, 0x0008, 7603},
    {"ProbeFacingDirection", 44, 1, 0x0014, 1705},
};

inline constexpr Field kFields_0368[] = {
    {"Delay", 24, 4, 0x0008, 7606},
    {"Scale", 28, 4, 0x0008, 7607},
    {"Offset", 32, 4, 0x0008, 7608},
    {"Distance", 36, 4, 0x0008, 7609},
    {"Depth", 40, 4, 0x0008, 7610},
    {"Direction", 44, 1, 0x0204, 2443},
};

inline constexpr Field kFields_0369[] = {
    {"AttachmentName", 24, 8, 0x0010, 0},
    {"ScaleJointName", 32, 8, 0x0010, 605},
    {"StartScale", 40, 4, 0x0008, 7613},
    {"TimeToHold", 44, 4, 0x0008, 7614},
    {"HoldScale", 48, 4, 0x0008, 7615},
    {"HoldDuration", 52, 4, 0x0008, 7616},
    {"TimeToEnd", 56, 4, 0x0008, 7617},
    {"EndScale", 60, 4, 0x0008, 7618},
    {"EaseType", 64, 1, 0x0104, 2448},
};

inline constexpr Field kFields_036A[] = {
    {"BlackboardCreatureVar", 24, 8, 0x0010, 0},
    {"ReadBlackboardOnInitOnly", 32, 1, 0x0014, 1706},
};

inline constexpr Field kFields_036C[] = {
    {"PrimaryJoint", 24, 8, 0x0010, 0},
    {"AttachName", 32, 8, 0x0010, 0},
    {"YPositionJoint", 40, 8, 0x0010, 0},
    {"Offset", 48, 0, 0x002C, 6},
    {"Rotation", 54, 0, 0x002C, 6},
    {"Radius", 60, 4, 0x0008, 7631},
    {"Length", 64, 4, 0x0008, 7632},
};

inline constexpr Field kFields_036E[] = {
    {"PropType", 24, 8, 0x0010, 0},
    {"AngleOffset", 32, 4, 0x0008, 7637},
    {"Distance", 36, 4, 0x0008, 7638},
    {"HeightOffset", 40, 4, 0x0008, 7639},
    {"LaunchSpeed", 44, 4, 0x0008, 7640},
    {"SpawnStatic", 48, 1, 0x0014, 1707},
};

inline constexpr Field kFields_036F[] = {
    {"PropType", 24, 8, 0x0010, 0},
    {"AttachJointParent", 32, 8, 0x0010, 612},
    {"ChildAttachJoint", 40, 8, 0x0010, 0},
    {"SynchedJoint", 48, 8, 0x0010, 0},
    {"AttachCarried", 56, 1, 0x0000, 963},
    {"CylinderHeight", 60, 4, 0x0008, 7643},
    {"CylinderRadius", 64, 4, 0x0008, 7644},
    {"CylinderOffsetX", 68, 4, 0x0008, 7645},
    {"CylinderOffsetZ", 72, 4, 0x0008, 7646},
};

inline constexpr Field kFields_0370[] = {
    {"DeleteOnDrop", 24, 1, 0x0014, 1708},
};

inline constexpr Field kFields_0372[] = {
    {"TargetCreatureFilter", 24, 0, 0x002C, 241},
};

inline constexpr Field kFields_0374[] = {
    {"MaxTargets", 24, 1, 0x0000, 969},
    {"MinTargets", 25, 1, 0x0000, 970},
    {"MaxDistanceForAutomatedTargets", 28, 4, 0x0008, 7657},
    {"MinOffsetDistance", 32, 4, 0x0008, 7658},
    {"MaxOffsetDistance", 36, 4, 0x0008, 7659},
    {"PersistTargetTime", 40, 4, 0x0008, 7660},
    {"SingleRetargetTime", 44, 4, 0x0008, 7661},
    {"MinTargetDistance", 48, 4, 0x0008, 7662},
    {"PaintTargetFlags", 52, 1, 0x0204, 2493},
    {"PaintControl", 53, 1, 0x0104, 2494},
};

inline constexpr Field kFields_0375[] = {
    {"HitJointName", 24, 8, 0x0010, 0},
    {"DefaultCollision", 32, 8, 0x001C, 396},
    {"WeaponType", 40, 4, 0x0000, 972},
    {"AngleThreshold", 44, 4, 0x0008, 7665},
};

inline constexpr Field kFields_0376[] = {
    {"PlayFXList", 24, 0, 0x002C, 273},
    {"Concussion", 40, 12, 0x0024, 424},
    {"ScaleX", 52, 4, 0x0008, 7668},
    {"TornadoGOName", 56, 8, 0x0018, 0},
    {"EnterAnimName", 64, 8, 0x0010, 0},
    {"ExitAnimName", 72, 8, 0x0010, 0},
    {"pointer_7", 80, 8, 0x001C, 268},
    {"ClearRadius", 88, 8, 0x001C, 721},
    {"ScaleY", 96, 4, 0x0008, 7669},
    {"ScaleZ", 100, 4, 0x0008, 7670},
    {"ExitAnimDuration", 104, 4, 0x0008, 7671},
    {"DurationMin", 108, 4, 0x0008, 7672},
    {"DurationMax", 112, 4, 0x0008, 7673},
    {"FadeTime", 116, 4, 0x0008, 7674},
    {"SliceAngle", 120, 4, 0x0008, 7675},
    {"StartDistance", 124, 4, 0x0008, 7676},
    {"Distance", 128, 4, 0x0008, 7677},
    {"SpeedMin", 132, 4, 0x0008, 7678},
    {"SpeedMax", 136, 4, 0x0008, 7679},
    {"TopSpeed", 140, 4, 0x0008, 7680},
    {"LaneSpeedMin", 144, 4, 0x0008, 7681},
    {"TargetRadius", 148, 4, 0x0008, 7682},
    {"TargetCooldown", 152, 4, 0x0008, 7683},
    {"TornadoCooldown", 156, 4, 0x0008, 7684},
    {"TargetAcceleration", 160, 4, 0x0008, 7685},
    {"TargetDeceleration", 164, 4, 0x0008, 7686},
    {"TargetAccelerationStart", 168, 4, 0x0008, 7687},
    {"LaneNoise", 172, 4, 0x0008, 7688},
    {"NoiseReductionDistance", 176, 4, 0x0008, 7689},
    {"SampleFrequency", 180, 4, 0x0008, 7690},
    {"FreeTurnSpeed", 184, 4, 0x0008, 7691},
    {"FreeNoise", 188, 4, 0x0008, 7692},
    {"FreeLeashLength", 192, 4, 0x0008, 7693},
    {"FreeNoiseSampleFrequency", 196, 4, 0x0008, 7694},
    {"ConcussionFrequency", 200, 4, 0x0008, 7695},
    {"Heap", 204, 4, 0x0000, 974},
    {"Count", 208, 1, 0x0000, 975},
    {"RandomConcussions", 209, 1, 0x0014, 1711},
};

inline constexpr Field kFields_0377[] = {
    {"WeaponType", 24, 4, 0x0000, 977},
    {"Target", 28, 1, 0x0104, 2507},
    {"Offset", 30, 0, 0x002C, 3},
    {"TrackTarget", 34, 1, 0x0014, 1712},
};

inline constexpr Field kFields_0378[] = {
    {"EffectName", 0, 8, 0x0018, 0},
    {"TargetEffectName", 8, 8, 0x0018, 0},
    {"Speed", 16, 4, 0x0008, 7700},
    {"FadeTime", 20, 4, 0x0008, 7701},
    {"Arc", 24, 4, 0x0008, 7702},
    {"IgnoreCollisionDistance", 28, 4, 0x0008, 7703},
    {"LifeTime", 32, 4, 0x0008, 7704},
    {"EmbedWeaponType", 36, 4, 0x0000, 978},
    {"EmbedWeaponMove", 40, 8, 0x001C, 1283},
    {"Concussion", 48, 8, 0x001C, 268},
    {"ConcussionEnvironment", 56, 8, 0x001C, 268},
    {"ConcussionCreature", 64, 8, 0x001C, 268},
    {"OnHitAction", 72, 8, 0x0010, 621},
    {"SoundAction", 80, 8, 0x001C, 177},
    {"SoundWindowAction", 88, 8, 0x001C, 972},
};

inline constexpr Field kFields_0379[] = {
    {"AreaConcussionList", 24, 12, 0x0024, 425},
    {"Heap", 36, 4, 0x0000, 980},
    {"AreaEffectGOName", 40, 8, 0x0010, 0},
    {"Projectile", 48, 8, 0x001C, 888},
    {"RandomAreaConcussion", 56, 8, 0x001C, 268},
    {"SoundAction", 64, 8, 0x001C, 177},
    {"SoundWindowAction", 72, 8, 0x001C, 972},
    {"SourceOffset", 80, 0, 0x002C, 6},
    {"TargetOffset", 86, 0, 0x002C, 6},
    {"SourceOffsetRandomRange", 92, 0, 0x002C, 6},
    {"NumArrows", 98, 1, 0x0000, 981},
    {"GuaranteedEnemyHits", 99, 1, 0x0000, 982},
    {"Radius", 100, 4, 0x0008, 7716},
    {"RainDuration", 104, 4, 0x0008, 7717},
    {"RainDelay", 108, 4, 0x0008, 7718},
    {"StartHeight", 112, 4, 0x0008, 7719},
    {"NoTargetDistance", 116, 4, 0x0008, 7720},
    {"GroundCheckOffsetUp", 120, 4, 0x0008, 7721},
    {"GroundCheckOffsetDown", 124, 4, 0x0008, 7722},
    {"RandomAreaConcussionsDuration", 128, 4, 0x0008, 7723},
    {"RandomAreaConcussionDelay", 132, 4, 0x0008, 7724},
    {"MaxGuaranteedSingleTargetHits", 136, 1, 0x0000, 983},
    {"ArrowRainFlags", 137, 1, 0x0204, 2512},
    {"NumRandomAreaConcussions", 138, 1, 0x0000, 984},
    {"AccessibilityHighlightCategory", 139, 1, 0x0104, 2513},
};

inline constexpr Field kFields_037B[] = {
    {"Projectile", 24, 8, 0x001C, 888},
    {"JointName", 32, 8, 0x0010, 0},
    {"TargetJointName", 40, 8, 0x0010, 0},
    {"Offset", 48, 0, 0x002C, 6},
    {"Direction", 54, 0, 0x002C, 6},
    {"Heap", 60, 4, 0x0000, 987},
    {"Lead", 64, 4, 0x0008, 7735},
    {"Distance", 68, 4, 0x0008, 7736},
};

inline constexpr Field kFields_037C[] = {
    {"Percent", 24, 4, 0x0008, 7739},
    {"RandomRange", 28, 4, 0x0008, 7740},
};

inline constexpr Field kFields_037D[] = {
    {"Resist", 24, 4, 0x0008, 7743},
};

inline constexpr Field kFields_037E[] = {
    {"CanBouncePendulum", 24, 1, 0x0014, 1713},
};

inline constexpr Field kFields_037F[] = {
    {"CanHitPendulum", 24, 1, 0x0014, 1714},
};

inline constexpr Field kFields_0380[] = {
    {"Beam", 24, 8, 0x001C, 412},
    {"SourceJointName", 32, 8, 0x0010, 630},
    {"SourceJointOffset", 40, 0, 0x002C, 6},
    {"SourceJointDirection", 46, 0, 0x002C, 6},
    {"EmitSpeed", 52, 4, 0x0008, 7756},
};

inline constexpr Field kFields_0381[] = {
    {"Beam", 24, 8, 0x001C, 412},
    {"SourceJointName", 32, 8, 0x0010, 632},
    {"TargetJointName", 40, 8, 0x0010, 633},
    {"SourceJointOffset", 48, 0, 0x002C, 6},
    {"NoTargetOffset", 54, 0, 0x002C, 6},
    {"PrepareTime", 60, 4, 0x0008, 7765},
    {"HitPlayerPauseTime", 64, 4, 0x0008, 7766},
    {"HitPauseTime", 68, 4, 0x0008, 7767},
    {"HorizontalAngleMin", 72, 4, 0x0008, 7768},
    {"HorizontalAngleMax", 76, 4, 0x0008, 7769},
    {"VerticalAngleMin", 80, 4, 0x0008, 7770},
    {"VerticalAngleMax", 84, 4, 0x0008, 7771},
    {"RotationSpeed", 88, 4, 0x0008, 7772},
    {"ChasingSpeed", 92, 4, 0x0008, 7773},
};

inline constexpr Field kFields_0383[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"NeckAngularAccel", 32, 4, 0x0008, 7793},
    {"NeckMaxAngularVel", 36, 4, 0x0008, 7794},
    {"MaxHeadLeftTurnAngle", 40, 4, 0x0008, 7795},
    {"MaxHeadRightTurnAngle", 44, 4, 0x0008, 7796},
    {"MaxHeadTiltFwdAngle", 48, 4, 0x0008, 7797},
    {"MaxHeadTiltBackAngle", 52, 4, 0x0008, 7798},
    {"TargetAngleOffset", 56, 4, 0x0008, 7799},
    {"ChestInfluence", 60, 4, 0x0008, 7800},
    {"HeadInfluence", 64, 4, 0x0008, 7801},
    {"HeadRollDamping", 68, 4, 0x0008, 7802},
    {"HeadPitchDamping", 72, 4, 0x0008, 7803},
    {"ChestPitchDamping", 76, 4, 0x0008, 7804},
    {"EaseInSpeed", 80, 4, 0x0008, 7805},
    {"EaseOutSpeed", 84, 4, 0x0008, 7806},
    {"NeckAngularAccel_IsNull", 88, 1, 0x0016, 1715},
    {"NeckMaxAngularVel_IsNull", 89, 1, 0x0016, 1716},
    {"MaxHeadLeftTurnAngle_IsNull", 90, 1, 0x0016, 1717},
    {"MaxHeadRightTurnAngle_IsNull", 91, 1, 0x0016, 1718},
    {"MaxHeadTiltFwdAngle_IsNull", 92, 1, 0x0016, 1719},
    {"MaxHeadTiltBackAngle_IsNull", 93, 1, 0x0016, 1720},
    {"TargetAngleOffset_IsNull", 94, 1, 0x0016, 1721},
    {"ChestInfluence_IsNull", 95, 1, 0x0016, 1722},
    {"HeadInfluence_IsNull", 96, 1, 0x0016, 1723},
    {"HeadRollDamping_IsNull", 97, 1, 0x0016, 1724},
    {"HeadPitchDamping_IsNull", 98, 1, 0x0016, 1725},
    {"ChestPitchDamping_IsNull", 99, 1, 0x0016, 1726},
    {"EaseInSpeed_IsNull", 100, 1, 0x0016, 1727},
    {"EaseOutSpeed_IsNull", 101, 1, 0x0016, 1728},
};

inline constexpr Field kFields_0384[] = {
    {"EaseOutSpeed", 24, 4, 0x0008, 7809},
};

inline constexpr Field kFields_0385[] = {
    {"AnimName", 24, 8, 0x0010, 0},
    {"LeftWristTarget", 32, 8, 0x0010, 0},
    {"RightWristTarget", 40, 8, 0x0010, 0},
    {"ReferenceJoint", 48, 8, 0x0010, 0},
    {"WeaponName", 56, 8, 0x0010, 0},
    {"WeaponIKJoint", 64, 8, 0x0010, 0},
    {"WeaponHolsterJoint", 72, 8, 0x0010, 0},
    {"ElbowResist", 80, 4, 0x0008, 7812},
    {"ShoulderResist", 84, 4, 0x0008, 7813},
    {"RightElbowTranslation", 88, 4, 0x0008, 7814},
    {"RightElbowRotation", 92, 4, 0x0008, 7815},
    {"LeftElbowTranslation", 96, 4, 0x0008, 7816},
    {"LeftElbowRotation", 100, 4, 0x0008, 7817},
    {"RightShoulderTranslation", 104, 4, 0x0008, 7818},
    {"RightShoulderRotation", 108, 4, 0x0008, 7819},
    {"LeftShoulderTranslation", 112, 4, 0x0008, 7820},
    {"LeftShoulderRotation", 116, 4, 0x0008, 7821},
    {"GravityDistance", 120, 4, 0x0008, 7822},
    {"HandleWeaponAnimation", 124, 1, 0x0014, 1729},
    {"SolveSteps", 128, 4, 0x0204, 2562},
};

inline constexpr Field kFields_0386[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_0387[] = {
    {"AnimName", 24, 8, 0x0010, 0},
    {"TweenTime", 32, 4, 0x0008, 7827},
};

inline constexpr Field kFields_0389[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"Exit", 32, 1, 0x0014, 1730},
    {"Exit_IsNull", 33, 1, 0x0016, 1731},
};

inline constexpr Field kFields_038A[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"HookName", 32, 8, 0x0018, 0},
    {"HookName_IsNull", 40, 1, 0x0016, 1732},
};

inline constexpr Field kFields_038B[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"EntryName", 32, 8, 0x0010, 0},
    {"ReadValueEntryName", 40, 8, 0x0010, 0},
    {"Duration", 48, 4, 0x0008, 7836},
    {"BlackboardType", 52, 1, 0x0104, 2587},
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1733},
    {"OnTarget", 54, 1, 0x0014, 1734},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1735},
    {"EntryName_IsNull", 56, 1, 0x0016, 1736},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1737},
    {"Duration_IsNull", 58, 1, 0x0016, 1738},
    {"ReadValueBlackboardType", 59, 1, 0x0104, 2588},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1739},
};

inline constexpr Field kFields_038C[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1740},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1742},
    {"EntryName_IsNull", 56, 1, 0x0016, 1743},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1744},
    {"Duration_IsNull", 58, 1, 0x0016, 1745},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1746},
};

inline constexpr Field kFields_038D[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1747},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1749},
    {"EntryName_IsNull", 56, 1, 0x0016, 1750},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1751},
    {"Duration_IsNull", 58, 1, 0x0016, 1752},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1753},
    {"ModFloatValue", 64, 0, 0x002C, 229},
    {"Value", 104, 4, 0x0008, 7849},
    {"Value_IsNull", 108, 1, 0x0016, 1754},
    {"ModFloatValue_IsNull", 109, 1, 0x0016, 1755},
};

inline constexpr Field kFields_038E[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1756},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1758},
    {"EntryName_IsNull", 56, 1, 0x0016, 1759},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1760},
    {"Duration_IsNull", 58, 1, 0x0016, 1761},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1762},
    {"Value", 64, 0, 0x002C, 9},
    {"Value_IsNull", 72, 1, 0x0016, 1763},
};

inline constexpr Field kFields_038F[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1764},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1766},
    {"EntryName_IsNull", 56, 1, 0x0016, 1767},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1768},
    {"Duration_IsNull", 58, 1, 0x0016, 1769},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1770},
    {"Value", 64, 1, 0x0014, 1771},
    {"Value_IsNull", 65, 1, 0x0016, 1772},
};

inline constexpr Field kFields_0390[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1773},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1775},
    {"EntryName_IsNull", 56, 1, 0x0016, 1776},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1777},
    {"Duration_IsNull", 58, 1, 0x0016, 1778},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1779},
    {"Value", 64, 8, 0x0010, 0},
    {"Value_IsNull", 72, 1, 0x0016, 1780},
};

inline constexpr Field kFields_0391[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1781},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1783},
    {"EntryName_IsNull", 56, 1, 0x0016, 1784},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1785},
    {"Duration_IsNull", 58, 1, 0x0016, 1786},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1787},
    {"Value", 64, 4, 0x0000, 1010},
    {"Value_IsNull", 68, 1, 0x0016, 1788},
};

inline constexpr Field kFields_0392[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1789},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1791},
    {"EntryName_IsNull", 56, 1, 0x0016, 1792},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1793},
    {"Duration_IsNull", 58, 1, 0x0016, 1794},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1795},
    {"ModFloatValue", 64, 0, 0x002C, 229},
    {"Value", 104, 4, 0x0008, 7875},
    {"Value_IsNull", 108, 1, 0x0016, 1796},
    {"ModFloatValue_IsNull", 109, 1, 0x0016, 1797},
};

inline constexpr Field kFields_0393[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1798},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1800},
    {"EntryName_IsNull", 56, 1, 0x0016, 1801},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1802},
    {"Duration_IsNull", 58, 1, 0x0016, 1803},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1804},
    {"SetToOneIfNull", 64, 1, 0x0014, 1805},
    {"SetToOneIfNull_IsNull", 65, 1, 0x0016, 1806},
};

inline constexpr Field kFields_0394[] = {
    {"BlackboardType_IsNull", 53, 1, 0x0016, 1807},
    {"OnTarget_IsNull", 55, 1, 0x0016, 1809},
    {"EntryName_IsNull", 56, 1, 0x0016, 1810},
    {"ReadValueEntryName_IsNull", 57, 1, 0x0016, 1811},
    {"Duration_IsNull", 58, 1, 0x0016, 1812},
    {"ReadValueBlackboardType_IsNull", 60, 1, 0x0016, 1813},
    {"ReduceBelowZero", 64, 1, 0x0014, 1814},
    {"ReduceBelowZero_IsNull", 65, 1, 0x0016, 1815},
};

inline constexpr Field kFields_0395[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"CreatureJoint", 32, 8, 0x0010, 655},
    {"InteractJoint", 40, 8, 0x0010, 656},
    {"InteractPhysicsObjectJoint", 48, 8, 0x0010, 657},
    {"Arrive", 56, 4, 0x0008, 7884},
    {"WeaponType", 60, 4, 0x0000, 1015},
    {"Arrive_IsNull", 64, 1, 0x0016, 1816},
    {"CreatureJoint_IsNull", 65, 1, 0x0016, 1817},
    {"InteractJoint_IsNull", 66, 1, 0x0016, 1818},
    {"InteractPhysicsObjectJoint_IsNull", 67, 1, 0x0016, 1819},
    {"WeaponType_IsNull", 68, 1, 0x0016, 1820},
    {"HideOnFinish", 69, 1, 0x0014, 1821},
    {"HideOnFinish_IsNull", 70, 1, 0x0016, 1822},
};

inline constexpr Field kFields_0396[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_0397[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_0398[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"Restore", 32, 4, 0x0008, 7891},
    {"Restore_IsNull", 36, 1, 0x0016, 1823},
};

inline constexpr Field kFields_0399[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_039A[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_039B[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"Mode", 32, 8, 0x0010, 0},
    {"Mode_IsNull", 40, 1, 0x0016, 1824},
};

inline constexpr Field kFields_039C[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"Mode", 32, 8, 0x0010, 0},
    {"Mode_IsNull", 40, 1, 0x0016, 1825},
};

inline constexpr Field kFields_039D[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_039E[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"Tags", 32, 12, 0x0024, 428},
    {"Tags_IsNull", 44, 1, 0x0016, 1826},
};

inline constexpr Field kFields_039F[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"Tags", 32, 12, 0x0024, 429},
    {"Tags_IsNull", 44, 1, 0x0016, 1827},
};

inline constexpr Field kFields_03A0[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
    {"Tags", 32, 12, 0x0024, 430},
    {"Tags_IsNull", 44, 1, 0x0016, 1828},
};

inline constexpr Field kFields_03A1[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_03A2[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_03A3[] = {
    {"TemplateSymbol", 24, 8, 0x001A, 0},
};

inline constexpr Field kFields_03A4[] = {
    {"Event", 24, 0, 0x002C, 180},
};

inline constexpr Field kFields_03A5[] = {
    {"Layer", 24, 1, 0x0104, 2717},
    {"Mode", 25, 1, 0x0104, 2718},
    {"RegionID", 26, 1, 0x0104, 2719},
    {"Value", 28, 4, 0x0008, 7919},
    {"ApplyTime", 32, 4, 0x0008, 7920},
    {"ApplyOn", 36, 1, 0x0104, 2720},
    {"CreatureID", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_03A6[] = {
    {"Layer", 24, 1, 0x0104, 2725},
};

inline constexpr Field kFields_03A7[] = {
    {"Layer", 24, 1, 0x0104, 2730},
};

inline constexpr Field kFields_03A8[] = {
    {"EmissiveScale", 24, 4, 0x0008, 7927},
    {"EaseInTime", 28, 4, 0x0008, 7928},
    {"EaseOutTime", 32, 4, 0x0008, 7929},
    {"ApplyToActiveWeaponsOnly", 36, 1, 0x0014, 1829},
};

inline constexpr Field kFields_03A9[] = {
    {"HealthBarState", 24, 1, 0x0104, 2739},
};

inline constexpr Field kFields_03AA[] = {
    {"ForceToShow", 0, 1, 0x0014, 1830},
    {"ShouldShow", 1, 1, 0x0014, 1831},
    {"RingRadius", 4, 4, 0x0008, 7932},
};

inline constexpr Field kFields_03AB[] = {
    {"Param", 24, 0, 0x002C, 938},
};

inline constexpr Field kFields_03AC[] = {
    {"ForceOn", 24, 1, 0x0014, 1834},
    {"HudElements", 25, 1, 0x0204, 2748},
};

inline constexpr Field kFields_03AD[] = {
    {"kickOffReactionMove", 24, 4, 0x0008, 7940},
};

inline constexpr Field kFields_03AE[] = {
    {"NextAnimationTotalFrames", 24, 1, 0x0000, 1040},
    {"PhaseOfWarpStart", 28, 4, 0x0008, 7943},
};

inline constexpr Field kFields_03AF[] = {
    {"StimName", 24, 8, 0x0018, 0},
    {"SendTo", 32, 1, 0x0104, 2761},
    {"CreatureID", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_03B1[] = {
    {"JointName", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_03B2[] = {
    {"AnimationWeightParams", 24, 12, 0x0024, 431},
    {"AnimationWeight", 36, 4, 0x0008, 7952},
    {"AttachmentObjectName", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_03B3[] = {
    {"AttachmentObjectName", 24, 8, 0x0010, 0},
    {"TimeUntilDeactivateSimulation", 32, 2, 0x0008, 7955},
};

inline constexpr Field kFields_03B4[] = {
    {"AttachmentObjectName", 24, 8, 0x0010, 0},
    {"CollideWithGround", 32, 1, 0x0014, 1835},
};

inline constexpr Field kFields_03B6[] = {
    {"Effect", 24, 8, 0x001C, 481},
    {"ParameterList", 32, 12, 0x0024, 432},
    {"JointName", 48, 8, 0x0010, 690},
    {"WeaponType", 56, 4, 0x0000, 1049},
    {"UseActionDuration", 60, 1, 0x0014, 1836},
};

inline constexpr Field kFields_03B7[] = {
    {"Effect", 24, 8, 0x001C, 494},
};

inline constexpr Field kFields_03B9[] = {
    {"ToggleType", 24, 1, 0x0104, 2802},
};

inline constexpr Field kFields_03BA[] = {
    {"AtLeastOneUpdate", 24, 1, 0x0014, 1837},
};

inline constexpr Field kFields_03BC[] = {
    {"CreatureCollision", 24, 1, 0x0014, 1838},
    {"GrabChainInHand", 25, 1, 0x0014, 1839},
};

inline constexpr Field kFields_03BD[] = {
    {"IconName", 24, 8, 0x0010, 0},
    {"OverrideHeight", 32, 4, 0x0008, 7976},
    {"BlendIn", 36, 4, 0x0008, 7977},
    {"BlendOut", 40, 4, 0x0008, 7978},
    {"Persistence", 44, 1, 0x0104, 2819},
};

inline constexpr Field kFields_03BF[] = {
    {"VFSEvents", 0, 12, 0x0024, 433},
};

inline constexpr Field kFields_03C0[] = {
    {"Setting", 0, 1, 0x0104, 2824},
    {"VFSSettingName", 8, 8, 0x0018, 0},
    {"RTPCName", 16, 8, 0x0018, 0},
};

inline constexpr Field kFields_03C1[] = {
    {"Bus", 0, 1, 0x0104, 2825},
    {"BusName", 8, 8, 0x0018, 0},
    {"RTPCName", 16, 8, 0x0018, 0},
};

inline constexpr Field kFields_03C2[] = {
    {"UpToPortalSize", 0, 4, 0x0008, 7981},
    {"MinBlendDistance", 4, 4, 0x0008, 7982},
    {"MaxBlendDistance", 8, 4, 0x0008, 7983},
};

inline constexpr Field kFields_03C3[] = {
    {"KeyPoints", 0, 12, 0x0024, 434},
};

inline constexpr Field kFields_03C4[] = {
    {"DefaultPortalBlendDistancesCurve", 0, 0, 0x002C, 963},
    {"SoundSettingRTPCs", 16, 12, 0x0024, 436},
    {"StreamingIOMemory", 28, 4, 0x0000, 1058},
    {"TargetAutoStreamBufferLength", 32, 4, 0x0008, 7984},
    {"StreamingGranularity", 36, 4, 0x0000, 1059},
    {"OcclusionReflectionSurfaceOffset", 40, 4, 0x0008, 7985},
    {"DistanceToCullPortals", 44, 4, 0x0008, 7986},
    {"SoundEmitterCullingRadius", 48, 4, 0x0008, 7987},
    {"LowerEngineMemSizeMB", 52, 4, 0x0000, 1060},
    {"HighEngineMemSizeMB", 56, 4, 0x0000, 1061},
    {"StreamingCacheMaxSizeKB", 60, 4, 0x0000, 1062},
    {"ObstructionRayCastsPerFrameSoftCap", 64, 2, 0x0000, 1063},
    {"ObstructionRayCastsPerFrameHardCap", 66, 2, 0x0000, 1064},
    {"UseStreamingCache", 68, 1, 0x0014, 1840},
};

inline constexpr Field kFields_03C5[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Environment", 8, 12, 0x0024, 437},
    {"StateGroup", 24, 8, 0x0018, 0},
    {"State", 32, 8, 0x0018, 0},
    {"Ambience", 40, 8, 0x0010, 0},
    {"WallOcclusion", 48, 4, 0x0008, 7988},
    {"ReverbVolumedB", 52, 4, 0x0008, 7989},
};

inline constexpr Field kFields_03C6[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"OcclusionOpen", 8, 4, 0x0008, 7990},
    {"OcclusionClosed", 12, 4, 0x0008, 7991},
    {"MaxDiffraction", 16, 4, 0x0008, 7992},
    {"MinDiffractionAngle", 20, 4, 0x0008, 7993},
    {"MaxDiffractionAngle", 24, 4, 0x0008, 7994},
    {"MinBlendDistanceScalar", 28, 4, 0x0008, 7995},
    {"MaxBlendDistanceScalar", 32, 4, 0x0008, 7996},
    {"InsidenessAtMinDistance", 36, 4, 0x0008, 7997},
    {"OcclusionOpenLPF", 40, 4, 0x0008, 7998},
    {"OcclusionOpenHPF", 44, 4, 0x0008, 7999},
    {"OcclusionClosedLPF", 48, 4, 0x0008, 8000},
    {"OcclusionClosedHPF", 52, 4, 0x0008, 8001},
    {"MaxDiffractionLPF", 56, 4, 0x0008, 8002},
    {"MaxDiffractionHPF", 60, 4, 0x0008, 8003},
    {"MinInsidenessDistanceScalar", 64, 4, 0x0008, 8004},
    {"MaxInsidenessDistanceScalar", 68, 4, 0x0008, 8005},
    {"MinVirtualPosDistanceScalar", 72, 4, 0x0008, 8006},
    {"MaxVirtualPosDistanceScalar", 76, 4, 0x0008, 8007},
    {"MinDiffractionDistanceScalar", 80, 4, 0x0008, 8008},
    {"MaxDiffractionDistanceScalar", 84, 4, 0x0008, 8009},
    {"OpenOcclusionBlendDistanceScalar", 88, 4, 0x0008, 8010},
    {"InsidenessAtMinDistanceSideA", 92, 4, 0x0008, 8011},
    {"InsidenessAtMinDistanceSideB", 96, 4, 0x0008, 8012},
    {"IsOpen", 100, 1, 0x0014, 1841},
    {"EnableVirtualPositions", 101, 1, 0x0014, 1842},
};

inline constexpr Field kFields_03C7[] = {
    {"Regions", 0, 12, 0x0024, 438},
    {"Portals", 16, 12, 0x0024, 439},
};

inline constexpr Field kFields_03C8[] = {
    {"Actor", 0, 8, 0x0018, 0},
    {"Sound", 8, 8, 0x0018, 0},
    {"MarkerID", 16, 2, 0x0000, 1065},
    {"Start", 20, 4, 0x0008, 8013},
    {"Gain", 24, 4, 0x0008, 8014},
    {"Inner", 28, 4, 0x0008, 8015},
    {"Outer", 32, 4, 0x0008, 8016},
    {"Move", 40, 8, 0x0018, 0},
    {"Animation", 48, 8, 0x0018, 0},
    {"On", 56, 4, 0x0008, 8017},
    {"Off", 60, 4, 0x0008, 8018},
};

inline constexpr Field kFields_03C9[] = {
    {"Cue", 0, 12, 0x0024, 440},
};

inline constexpr Field kFields_03CA[] = {
    {"EventType", 24, 1, 0x0104, 2830},
};

inline constexpr Field kFields_03CE[] = {
    {"Hand", 72, 1, 0x0104, 2855},
    {"Doppler", 76, 4, 0x0008, 8041},
};

inline constexpr Field kFields_03CF[] = {
    {"SoundEvent", 72, 8, 0x0018, 0},
    {"DoNotStartIfAlreadyPlaying", 80, 1, 0x0014, 1848},
};

inline constexpr Field kFields_03D0[] = {
    {"SoundEvent", 72, 8, 0x0018, 0},
};

inline constexpr Field kFields_03D1[] = {
    {"Next", 72, 8, 0x0010, 0},
};

inline constexpr Field kFields_03D2[] = {
    {"Name", 8, 8, 0x0018, 0},
    {"Time", 16, 4, 0x0008, 8059},
    {"StartFlags", 20, 1, 0x0204, 2877},
};

inline constexpr Field kFields_03D5[] = {
    {"Time", 8, 4, 0x0008, 8068},
};

inline constexpr Field kFields_03D6[] = {
    {"CutIndex", 0, 1, 0x0004, 2889},
    {"EventName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_03D7[] = {
    {"Events", 24, 12, 0x0024, 441},
};

inline constexpr Field kFields_03D8[] = {
    {"RTPCName", 8, 8, 0x0018, 0},
    {"RTPCValue", 16, 4, 0x0008, 8073},
    {"MarkerID", 20, 2, 0x0000, 1080},
};

inline constexpr Field kFields_03D9[] = {
    {"StateGroup", 8, 8, 0x0018, 0},
    {"StateName", 16, 8, 0x0018, 0},
};

inline constexpr Field kFields_03DA[] = {
    {"MarkerID", 8, 2, 0x0000, 1083},
    {"SwitchGroup", 16, 8, 0x0018, 0},
    {"SwitchState", 24, 8, 0x0018, 0},
    {"EmitterName", 32, 8, 0x0018, 0},
};

inline constexpr Field kFields_03DB[] = {
    {"Name", 8, 8, 0x0010, 0},
    {"SoundEventOverride", 16, 8, 0x0010, 0},
    {"TriggerRangeOverride", 24, 4, 0x0008, 8080},
};

inline constexpr Field kFields_03DC[] = {
    {"Name", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_03DD[] = {
    {"Presets", 0, 0, 0x0028, 23},
};

inline constexpr Field kFields_03DE[] = {
    {"MidPoint", 16, 4, 0x0008, 8087},
};

inline constexpr Field kFields_03DF[] = {
    {"Cap", 0, 4, 0x0008, 8088},
    {"Up", 4, 4, 0x0008, 8089},
    {"Down", 8, 4, 0x0008, 8090},
    {"Damp", 12, 4, 0x0008, 8091},
};

inline constexpr Field kFields_03E0[] = {
    {"UserMode", 0, 1, 0x0104, 2909},
    {"ControlType", 1, 1, 0x0104, 2910},
    {"HorizontalMin", 4, 4, 0x0008, 8092},
    {"HorizontalMax", 8, 4, 0x0008, 8093},
    {"VerticalMin", 12, 4, 0x0008, 8094},
    {"VerticalMax", 16, 4, 0x0008, 8095},
};

inline constexpr Field kFields_03E1[] = {
    {"SelectFrame", 0, 0, 0x002C, 85},
    {"LockFrame", 16, 0, 0x002C, 85},
    {"AimSelectFrame", 32, 0, 0x002C, 85},
    {"AimLockFrame", 48, 0, 0x002C, 85},
};

inline constexpr Field kFields_03E2[] = {
    {"Recenter", 0, 0, 0x002C, 79},
    {"AutoRecenter", 152, 0, 0x002C, 79},
    {"DefaultFrames", 304, 0, 0x002C, 993},
    {"CreatureFrames", 368, 0, 0x0028, 24},
    {"ActivationRange", 380, 4, 0x0008, 8202},
    {"MaxRange", 384, 4, 0x0008, 8203},
    {"DistanceAngleToRangeNear", 388, 4, 0x0008, 8204},
    {"WeightAngleToRangeAimingNear", 392, 4, 0x0008, 8205},
    {"WeightAngleToRangeNear", 396, 4, 0x0008, 8206},
    {"DistanceAngleToRangeFar", 400, 4, 0x0008, 8207},
    {"WeightAngleToRangeAimingFar", 404, 4, 0x0008, 8208},
    {"WeightAngleToRangeFar", 408, 4, 0x0008, 8209},
    {"ActivationMinTime", 412, 4, 0x0008, 8210},
    {"InitialFlickDelay", 416, 4, 0x0008, 8211},
    {"MaxHiddenTime", 420, 4, 0x0008, 8212},
    {"DeathHoldTime", 424, 4, 0x0008, 8213},
    {"StickFullOn", 428, 4, 0x0008, 8214},
    {"StickRelease", 432, 4, 0x0008, 8215},
    {"FlickMaxTime", 436, 4, 0x0008, 8216},
    {"StickFullOnAiming", 440, 4, 0x0008, 8217},
    {"FlickWeightAngleToRange", 444, 4, 0x0008, 8218},
    {"FlickDirectionAngleFilter", 448, 4, 0x0008, 8219},
    {"InvisibleReacquireTimeOut", 452, 4, 0x0008, 8220},
    {"AimLockFilterStickAngle", 456, 4, 0x0008, 8221},
    {"RotationSpeedLimit", 460, 4, 0x0008, 8222},
    {"RotationAccelerationLimit", 464, 4, 0x0008, 8223},
    {"RotationDamping", 468, 4, 0x0008, 8224},
    {"PitchCloseSpeedLimit", 472, 4, 0x0008, 8225},
    {"PitchCloseAccelerationLimit", 476, 4, 0x0008, 8226},
    {"PitchCloseDamping", 480, 4, 0x0008, 8227},
    {"YawCloseSpeedLimit", 484, 4, 0x0008, 8228},
    {"YawCloseAccelerationLimit", 488, 4, 0x0008, 8229},
    {"YawCloseDamping", 492, 4, 0x0008, 8230},
    {"WeightAngleToRange", 496, 4, 0x0008, 8231},
    {"WeightAngleToRangeAiming", 500, 4, 0x0008, 8232},
    {"FlickUseCameraInvert", 504, 1, 0x0014, 1851},
    {"FlickToReacquireEvade", 505, 1, 0x0014, 1852},
    {"PrioritizeAttackers", 506, 1, 0x0104, 2915},
    {"CreaturesUseRenderCull", 507, 1, 0x0014, 1853},
    {"CreaturesUseAICull", 508, 1, 0x0014, 1854},
    {"CreaturesUseCollisionCull", 509, 1, 0x0014, 1855},
    {"AimLockMode", 510, 1, 0x0104, 2916},
};

inline constexpr Field kFields_03E3[] = {
    {"RigName", 0, 8, 0x0018, 701},
    {"DefaultTweenTime", 8, 4, 0x0008, 8233},
    {"QuickPreviewCamera", 16, 12, 0x0024, 442},
    {"CompassRecenter", 28, 0, 0x002C, 79},
    {"LockOn", 184, 0, 0x002C, 994},
    {"AimAssistRange", 696, 0, 0x002C, 990},
    {"ZoomSnapRange", 716, 0, 0x002C, 75},
    {"ZoomSnapWeapon", 736, 0, 0x0028, 26},
    {"ZoomSnapRecenter", 748, 0, 0x002C, 79},
    {"ZoomSnapStickCancelZone", 900, 4, 0x0008, 8439},
    {"ZoomSnapCloseFarThreshold", 904, 4, 0x0008, 8440},
    {"ZoomSnapCloseNearThreshold", 908, 4, 0x0008, 8441},
    {"ZoomSnapCloseUpLimit", 912, 4, 0x0008, 8442},
    {"ZoomSnapCloseDownLimit", 916, 4, 0x0008, 8443},
    {"SAFPlayerTargetOverdrive", 920, 4, 0x0008, 8444},
    {"SAFMinForward", 924, 4, 0x0008, 8445},
    {"SAFMaxForward", 928, 4, 0x0008, 8446},
    {"SAFMinBackward", 932, 4, 0x0008, 8447},
    {"SAFMaxBackward", 936, 4, 0x0008, 8448},
    {"SAFStickWeight", 940, 4, 0x0008, 8449},
    {"SAFSingleAlwaysAssist", 944, 1, 0x0014, 1861},
    {"SAFGroupAlwaysAssist", 945, 1, 0x0014, 1862},
    {"SAFProximityGuardInner", 948, 4, 0x0008, 8450},
    {"SAFProximityGuardOuter", 952, 4, 0x0008, 8451},
    {"SAFProximityTowardsScale", 956, 4, 0x0008, 8452},
    {"SAFProximityTowardsMax", 960, 4, 0x0008, 8453},
    {"SAFProScale", 964, 4, 0x0008, 8454},
    {"SAFConScale", 968, 4, 0x0008, 8455},
    {"IsPlayerTarget", 972, 0, 0x002C, 991},
    {"IsTargetingPlayer", 988, 0, 0x002C, 991},
    {"IsOnScreen", 1004, 0, 0x002C, 991},
    {"IsInFrame", 1020, 0, 0x002C, 991},
    {"TargetObscuredTimer", 1036, 4, 0x0008, 8472},
    {"TargetObscuredDamping", 1040, 4, 0x0008, 8473},
    {"TargetInterestAngleMin", 1044, 4, 0x0008, 8474},
    {"TargetInterestAngleMax", 1048, 4, 0x0008, 8475},
    {"MaxBokehSize", 1052, 4, 0x0008, 8476},
    {"BokehDepthCutoff", 1056, 4, 0x0008, 8477},
    {"BokehFalloff", 1060, 4, 0x0008, 8478},
    {"BokehBrightnessThreshold", 1064, 4, 0x0008, 8479},
    {"BokehBlurThreshold", 1068, 4, 0x0008, 8480},
    {"FilmBackWidthInches", 1072, 4, 0x0008, 8481},
    {"FilmBackHeightInches", 1076, 4, 0x0008, 8482},
    {"FilmBackSensorCoC", 1080, 4, 0x0008, 8483},
    {"LookDollySpeed", 1084, 4, 0x0008, 8484},
    {"LookDollyAcceleration", 1088, 4, 0x0008, 8485},
    {"LookDollyInvert", 1092, 1, 0x0000, 1086},
    {"SoftConstraintLimit", 1096, 4, 0x0008, 8486},
    {"SoftConstraintMinimum", 1100, 4, 0x0008, 8487},
    {"HoldR3Timer", 1104, 4, 0x0008, 8488},
    {"TiltRecenter", 1108, 0, 0x002C, 79},
    {"ManualRecenterMaxAngle", 1260, 4, 0x0008, 8526},
    {"ManualRecenter", 1264, 0, 0x002C, 79},
    {"DebugLook", 1416, 0, 0x002C, 80},
    {"CinematicLook", 1568, 0, 0x002C, 80},
    {"UserLook", 1720, 0, 0x002C, 80},
    {"MoveAnticipation", 1872, 0, 0x002C, 80},
    {"TargetLockRefractoryTimer", 2024, 4, 0x0008, 8684},
    {"TargetCapFalloff", 2028, 4, 0x0008, 8685},
    {"TargetCapFalloffEaseIn", 2032, 4, 0x0008, 8686},
    {"TargetCapFalloffEaseOut", 2036, 4, 0x0008, 8687},
    {"DefaultCamera", 2040, 12, 0x0024, 443},
    {"PlayerDefault", 2056, 0, 0x0028, 27},
    {"DebugCamera", 2072, 8, 0x0018, 0},
    {"DefaultEffects", 2080, 12, 0x0024, 444},
    {"TargetActivatedWeightIn", 2092, 4, 0x0008, 8688},
    {"TargetActivatedWeightOut", 2096, 4, 0x0008, 8689},
    {"TargetActivatedEaseIn", 2100, 4, 0x0008, 8690},
    {"TargetActivatedEaseOut", 2104, 4, 0x0008, 8691},
    {"TargetActivatedHysteresis", 2108, 4, 0x0008, 8692},
    {"TargetWarpTimer", 2112, 4, 0x0008, 8693},
    {"TargetWarpEaseIn", 2116, 4, 0x0008, 8694},
    {"TargetWarpEaseOut", 2120, 4, 0x0008, 8695},
    {"JumpCorrectionDoubleDecay", 2124, 4, 0x0008, 8696},
    {"JumpCorrectionLandDecay", 2128, 4, 0x0008, 8697},
    {"AggressionMode", 2132, 1, 0x0104, 2943},
    {"AggressionRampUp", 2136, 4, 0x0008, 8698},
    {"AggressionRampDown", 2140, 4, 0x0008, 8699},
    {"AggressionLimit", 2144, 4, 0x0008, 8700},
    {"AggressionThreshold", 2148, 4, 0x0008, 8701},
    {"AggressionEaseIn", 2152, 4, 0x0008, 8702},
    {"AggressionEaseOut", 2156, 4, 0x0008, 8703},
    {"PlayerCapsuleRadius", 2160, 4, 0x0008, 8704},
    {"PlayerCapsuleHeight", 2164, 4, 0x0008, 8705},
    {"PlayerCapsuleSlopeBase", 2168, 4, 0x0008, 8706},
    {"PlayerCapsuleSlopeTop", 2172, 4, 0x0008, 8707},
    {"DefaultPlayerCapsule", 2176, 0, 0x0028, 28},
    {"SoftFrustumCollisionDivergence", 2188, 4, 0x0008, 8708},
    {"PlayerRotationDamping", 2192, 4, 0x0008, 8709},
    {"PlayerRotationVelocityMax", 2196, 4, 0x0008, 8710},
    {"PlayerRotationAccelerationMax", 2200, 4, 0x0008, 8711},
    {"BehaviourBaseFocalLength", 2204, 4, 0x0008, 8712},
    {"CollisionMinimumDistanceFactor", 2208, 4, 0x0008, 8713},
    {"CollisionCapsuleThresholdDistance", 2212, 4, 0x0008, 8714},
    {"CollisionDamping", 2216, 4, 0x0008, 8715},
    {"HardConstraintDefaultClosed", 2220, 1, 0x0014, 1867},
    {"RotationToDistanceRecenterFactor", 2224, 4, 0x0008, 8716},
    {"MaxRotateToTargetSpeed", 2228, 4, 0x0008, 8717},
    {"FrictionRotationVelocityMax", 2232, 4, 0x0008, 8718},
    {"CombatLogicalTargetDamping", 2236, 4, 0x0008, 8719},
    {"CombatLogicalTargetDampingRadius", 2240, 4, 0x0008, 8720},
    {"ControlScale", 2248, 12, 0x0024, 445},
    {"PenetratingCreatureBuffer", 2260, 4, 0x0008, 8721},
    {"MaxBehaviourDisplacement", 2264, 4, 0x0008, 8722},
    {"CollisionTimeForward", 2268, 4, 0x0008, 8723},
    {"CollisionTimeBackward", 2272, 4, 0x0008, 8724},
    {"UICameraSpeed", 2276, 4, 0x0008, 8725},
    {"UICameraSpeedBlend", 2280, 4, 0x0008, 8726},
};

inline constexpr Field kFields_03E4[] = {
    {"DirectorySize", 0, 4, 0x0000, 1095},
    {"States", 4, 4, 0x0000, 1096},
    {"Rigs", 8, 4, 0x0000, 1097},
    {"MutableHeapSize", 12, 4, 0x0000, 1098},
};

inline constexpr Field kFields_03E5[] = {
    {"AnimationName", 80, 8, 0x0018, 0},
    {"Main", 88, 4, 0x0008, 8741},
    {"Translation", 92, 4, 0x0008, 8742},
    {"Rotation", 108, 4, 0x0008, 8746},
    {"Pitch", 112, 4, 0x0008, 8747},
    {"Yaw", 116, 4, 0x0008, 8748},
    {"Roll", 120, 4, 0x0008, 8749},
    {"Speed", 124, 4, 0x0008, 8750},
    {"FocalLength", 128, 4, 0x0008, 8751},
    {"BaseFocalLength", 132, 4, 0x0008, 8752},
    {"AbsoluteFocalLength", 136, 4, 0x0008, 8753},
    {"RelativeFocalLength", 140, 4, 0x0008, 8754},
    {"Additive", 144, 1, 0x0014, 1869},
    {"Mandatory", 145, 1, 0x0014, 1870},
    {"VelocityHigh", 148, 4, 0x0008, 8755},
    {"VelocityLow", 152, 4, 0x0008, 8756},
    {"StrengthHigh", 156, 4, 0x0008, 8757},
    {"StrengthLow", 160, 4, 0x0008, 8758},
    {"TimeUp", 164, 4, 0x0008, 8759},
    {"TimeDown", 168, 4, 0x0008, 8760},
    {"UserStrength", 172, 4, 0x0008, 8761},
    {"Inner", 176, 4, 0x0008, 8762},
    {"Outer", 180, 4, 0x0008, 8763},
    {"Joint", 184, 8, 0x0010, 0},
};

inline constexpr Field kFields_03E6[] = {
    {"PlayerCapsuleRadius", 80, 4, 0x0008, 8778},
    {"PlayerCapsuleHeight", 84, 4, 0x0008, 8779},
    {"PlayerCapsuleSlopeBase", 88, 4, 0x0008, 8780},
    {"PlayerCapsuleSlopeTop", 92, 4, 0x0008, 8781},
};

inline constexpr Field kFields_03E7[] = {
    {"Amount", 24, 2, 0x0008, 8784},
    {"Duration", 26, 2, 0x0008, 8785},
    {"TweenIn", 28, 0, 0x002C, 12},
    {"TweenOut", 52, 0, 0x002C, 12},
};

inline constexpr Field kFields_03E8[] = {
    {"Object", 24, 8, 0x0018, 0},
    {"Name", 32, 8, 0x0018, 0},
    {"TweenIn", 40, 0, 0x002C, 12},
    {"TweenOut", 64, 0, 0x002C, 12},
    {"TweenOutOverAnimation", 88, 1, 0x0014, 1872},
    {"KeepWorldSpace", 89, 1, 0x0014, 1873},
    {"IgnoreCollision", 90, 1, 0x0014, 1874},
    {"TweenOutIgnoreRotation", 91, 1, 0x0014, 1875},
    {"TweenOutIgnoreYaw", 92, 1, 0x0014, 1876},
    {"TweenOutIgnorePitch", 93, 1, 0x0014, 1877},
    {"LinearTween", 94, 1, 0x0014, 1878},
    {"PhotoSensitive", 95, 1, 0x0014, 1879},
    {"SeamlessCut", 96, 1, 0x0014, 1880},
};

inline constexpr Field kFields_03E9[] = {
    {"SynchJoint", 104, 8, 0x0010, 706},
    {"Priority", 112, 2, 0x0000, 1106},
    {"TweenPriority", 114, 2, 0x0000, 1107},
    {"Amount", 116, 2, 0x0008, 8826},
    {"Shake", 118, 2, 0x0008, 8827},
    {"SelfTweenTime", 120, 2, 0x0008, 8828},
};

inline constexpr Field kFields_03EA[] = {
    {"ApproachTweenIn", 128, 0, 0x002C, 12},
    {"ControlTweenIn", 152, 0, 0x002C, 12},
    {"TemplateSymbol", 176, 8, 0x001A, 0},
    {"ApproachYawCorrection", 184, 4, 0x0008, 8858},
    {"ApproachYawWorld", 188, 4, 0x0008, 8859},
    {"ApproachTweenIn_IsNull", 192, 1, 0x0016, 1899},
    {"ControlTweenIn_IsNull", 193, 1, 0x0016, 1900},
    {"ApproachYawCorrection_IsNull", 194, 1, 0x0016, 1901},
    {"ApproachYawWorld_IsNull", 195, 1, 0x0016, 1902},
};

inline constexpr Field kFields_03EB[] = {
    {"Enable", 8, 1, 0x0000, 1112},
    {"Name", 16, 8, 0x0018, 0},
};

inline constexpr Field kFields_03ED[] = {
    {"Recenter", 8, 0, 0x002C, 79},
    {"TemplateSymbol", 160, 8, 0x001A, 0},
    {"Facing", 168, 0, 0x002C, 6},
    {"Lock_IsNull", 174, 1, 0x0016, 1903},
    {"Hold", 175, 1, 0x0014, 1904},
    {"Lock", 176, 4, 0x0008, 8906},
    {"Pitch", 180, 4, 0x0008, 8907},
    {"Yaw", 184, 4, 0x0008, 8908},
    {"Hold_IsNull", 188, 1, 0x0016, 1905},
    {"Recenter_IsNull", 189, 1, 0x0016, 1906},
    {"Pitch_IsNull", 190, 1, 0x0016, 1907},
    {"Yaw_IsNull", 191, 1, 0x0016, 1908},
    {"Facing_IsNull", 192, 1, 0x0016, 1909},
};

inline constexpr Field kFields_03EE[] = {
    {"Target", 8, 8, 0x0018, 0},
    {"Recenter", 16, 0, 0x002C, 79},
    {"Lock", 168, 4, 0x0008, 8948},
    {"Hold", 172, 1, 0x0014, 1910},
    {"User", 173, 1, 0x0014, 1911},
    {"Horizontal", 174, 1, 0x0014, 1912},
    {"Vertical", 175, 1, 0x0014, 1913},
    {"UseReticle", 176, 1, 0x0014, 1914},
    {"Frame", 180, 0, 0x002C, 85},
};

inline constexpr Field kFields_03EF[] = {
    {"Activate", 200, 1, 0x0014, 1920},
};

inline constexpr Field kFields_03F3[] = {
    {"CameraFlags", 8, 1, 0x0204, 2999},
};

inline constexpr Field kFields_03F5[] = {
    {"Camera", 0, 0, 0x0028, 29},
    {"Priority", 12, 4, 0x0000, 1123},
};

inline constexpr Field kFields_03F6[] = {
    {"Left", 0, 4, 0x0008, 9049},
    {"Right", 4, 4, 0x0008, 9050},
    {"Top", 8, 4, 0x0008, 9051},
    {"Bottom", 12, 4, 0x0008, 9052},
};

inline constexpr Field kFields_03F7[] = {
    {"Transition", 0, 12, 0x0024, 446},
};

inline constexpr Field kFields_03F8[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"CursorGOName", 8, 8, 0x0018, 0},
    {"CollisionRoot", 16, 8, 0x0018, 0},
    {"HorizontalPosition", 24, 4, 0x0008, 9053},
    {"VerticalPosition", 28, 4, 0x0008, 9054},
    {"HeightOffset", 32, 4, 0x0008, 9055},
    {"ZoomPosition", 36, 4, 0x0008, 9056},
    {"HorizontalVelocity", 40, 4, 0x0008, 9057},
    {"VerticalVelocity", 44, 4, 0x0008, 9058},
    {"ZoomVelocity", 48, 4, 0x0008, 9059},
    {"Pitch", 52, 4, 0x0008, 9060},
    {"Yaw", 56, 4, 0x0008, 9061},
    {"Roll", 60, 4, 0x0008, 9062},
    {"ZoomSpeedScale", 64, 4, 0x0008, 9063},
    {"MaxLeft", 68, 4, 0x0008, 9064},
    {"MaxRight", 72, 4, 0x0008, 9065},
    {"MaxUp", 76, 4, 0x0008, 9066},
    {"MaxDown", 80, 4, 0x0008, 9067},
    {"MaxIn", 84, 4, 0x0008, 9068},
    {"MaxOut", 88, 4, 0x0008, 9069},
    {"HorizontalCameraOffset", 92, 4, 0x0008, 9070},
    {"VerticalCameraOffset", 96, 4, 0x0008, 9071},
    {"HorizontalControlSpeed", 100, 4, 0x0008, 9072},
    {"VerticalControlSpeed", 104, 4, 0x0008, 9073},
    {"ZoomControlSpeed", 108, 4, 0x0008, 9074},
    {"CursorSpeed", 112, 4, 0x0008, 9075},
    {"CursorPosition_H", 116, 4, 0x0008, 9076},
    {"CursorPosition_V", 120, 4, 0x0008, 9077},
    {"CursorPosition_H_Min", 124, 4, 0x0008, 9078},
    {"CursorPosition_H_Max", 128, 4, 0x0008, 9079},
    {"CursorPosition_V_Min", 132, 4, 0x0008, 9080},
    {"CursorPosition_V_Max", 136, 4, 0x0008, 9081},
    {"CursorSnap_Strength", 140, 4, 0x0008, 9082},
    {"CursorSnap_Distance", 144, 4, 0x0008, 9083},
    {"CursorSnap_PredictionStrength", 148, 4, 0x0008, 9084},
    {"CursorSnapMap_FalloffRadius", 152, 4, 0x0008, 9085},
    {"CursorScale_Min", 156, 4, 0x0008, 9086},
    {"CursorScale_Max", 160, 4, 0x0008, 9087},
    {"OnSkillTree", 164, 1, 0x0014, 1926},
    {"DetachCursorFromCamera", 165, 1, 0x0014, 1927},
    {"CursorSnap_Enabled", 166, 1, 0x0014, 1928},
    {"ScaleMapCursorBasedOnZoom", 167, 1, 0x0014, 1929},
};

inline constexpr Field kFields_03F9[] = {
    {"Name", 24, 8, 0x0018, 0},
    {"Set", 32, 8, 0x0010, 0},
    {"Suppress", 40, 1, 0x0014, 1930},
};

inline constexpr Field kFields_03FA[] = {
    {"CameraType_IsNull", 711, 1, 0x0016, 1932},
    {"ParentRelative_IsNull", 713, 1, 0x0016, 1934},
    {"Aggression_IsNull", 715, 1, 0x0016, 1936},
    {"TargetActivated_IsNull", 717, 1, 0x0016, 1938},
    {"Collision_IsNull", 719, 1, 0x0016, 1940},
    {"AvoidEverything_IsNull", 721, 1, 0x0016, 1942},
    {"DontCheckpoint_IsNull", 723, 1, 0x0016, 1944},
    {"StartCentered_IsNull", 725, 1, 0x0016, 1946},
    {"RegisterName_IsNull", 727, 1, 0x0016, 1948},
    {"PhotoSensitive_IsNull", 729, 1, 0x0016, 1950},
    {"ConstraintFriction_IsNull", 731, 1, 0x0016, 1951},
    {"Position_IsNull", 732, 1, 0x0016, 1952},
    {"Yaw_IsNull", 733, 1, 0x0016, 1953},
    {"Pitch_IsNull", 734, 1, 0x0016, 1954},
    {"Roll_IsNull", 735, 1, 0x0016, 1955},
    {"focalLength_IsNull", 736, 1, 0x0016, 1956},
    {"AngleOfView_IsNull", 737, 1, 0x0016, 1957},
    {"FocusDistance_IsNull", 738, 1, 0x0016, 1958},
    {"FStop_IsNull", 739, 1, 0x0016, 1959},
    {"LensDistortion_IsNull", 740, 1, 0x0016, 1960},
    {"DepthOfField_IsNull", 742, 1, 0x0016, 1962},
    {"IsOrthographic_IsNull", 744, 1, 0x0016, 1964},
    {"OrthoWidth_IsNull", 745, 1, 0x0016, 1965},
    {"StateFilter_IsNull", 746, 1, 0x0016, 1966},
    {"RequireMarker_IsNull", 747, 1, 0x0016, 1967},
    {"IgnoreMarker_IsNull", 748, 1, 0x0016, 1968},
    {"DefaultTweenTime_IsNull", 749, 1, 0x0016, 1969},
    {"DefaultTweenDistance_IsNull", 750, 1, 0x0016, 1970},
    {"DefaultEaseIn_IsNull", 751, 1, 0x0016, 1971},
    {"DefaultEaseOut_IsNull", 752, 1, 0x0016, 1972},
    {"DefaultLengthIn_IsNull", 753, 1, 0x0016, 1973},
    {"DefaultLengthOut_IsNull", 754, 1, 0x0016, 1974},
    {"TweenOverrides_IsNull", 755, 1, 0x0016, 1975},
    {"Transition_IsNull", 756, 1, 0x0016, 1976},
    {"SplineTarget_IsNull", 757, 1, 0x0016, 1977},
    {"TargetCap_IsNull", 758, 1, 0x0016, 1978},
    {"MinimumTargetPriority_IsNull", 759, 1, 0x0016, 1979},
    {"MinimumTargetActivatedPriority_IsNull", 760, 1, 0x0016, 1980},
    {"TargetMatches_IsNull", 761, 1, 0x0016, 1981},
    {"StrafeAssistTargetMatches_IsNull", 762, 1, 0x0016, 1982},
    {"TargetActivatedMatches_IsNull", 763, 1, 0x0016, 1983},
    {"RotateToTargetMatches_IsNull", 764, 1, 0x0016, 1984},
    {"TiltTargetMatches_IsNull", 765, 1, 0x0016, 1985},
    {"TargetFilter_IsNull", 767, 1, 0x0016, 1986},
    {"BoomDamping_IsNull", 768, 1, 0x0016, 1987},
    {"VerticalDamping_IsNull", 769, 1, 0x0016, 1988},
    {"HorizontalDamping_IsNull", 770, 1, 0x0016, 1989},
    {"DampingForward_IsNull", 771, 1, 0x0016, 1990},
    {"DampingBackward_IsNull", 772, 1, 0x0016, 1991},
    {"DampingLeft_IsNull", 773, 1, 0x0016, 1992},
    {"DampingRight_IsNull", 774, 1, 0x0016, 1993},
    {"DampingUp_IsNull", 775, 1, 0x0016, 1994},
    {"DampingDown_IsNull", 776, 1, 0x0016, 1995},
    {"MinVelocity_IsNull", 777, 1, 0x0016, 1996},
    {"MaxVelocity_IsNull", 778, 1, 0x0016, 1997},
    {"VelocityEaseIn_IsNull", 779, 1, 0x0016, 1998},
    {"VelocityEaseOut_IsNull", 780, 1, 0x0016, 1999},
    {"VelocityDampingTime_IsNull", 781, 1, 0x0016, 2000},
    {"Effect_IsNull", 782, 1, 0x0016, 2001},
    {"OrbitControl_IsNull", 783, 1, 0x0016, 2002},
    {"TiltControl_IsNull", 784, 1, 0x0016, 2003},
    {"OrbitConstraint_IsNull", 785, 1, 0x0016, 2004},
    {"TiltConstraint_IsNull", 786, 1, 0x0016, 2005},
    {"Recenter_IsNull", 787, 1, 0x0016, 2006},
    {"AutoRecenter_IsNull", 788, 1, 0x0016, 2007},
    {"TiltRecenter_IsNull", 789, 1, 0x0016, 2008},
    {"Animation_IsNull", 790, 1, 0x0016, 2009},
    {"AnimationRate_IsNull", 791, 1, 0x0016, 2010},
    {"TriggerAnimation_IsNull", 793, 1, 0x0016, 2012},
    {"ForceAnimation_IsNull", 795, 1, 0x0016, 2014},
    {"BoomRatio_IsNull", 796, 1, 0x0016, 2015},
    {"MaxDistanceToDolly_IsNull", 797, 1, 0x0016, 2016},
    {"MinDistanceToDolly_IsNull", 798, 1, 0x0016, 2017},
    {"MaxDistanceToTarget_IsNull", 799, 1, 0x0016, 2018},
    {"MinDistanceToTarget_IsNull", 800, 1, 0x0016, 2019},
    {"ElevationConstraint_IsNull", 801, 1, 0x0016, 2020},
    {"RotationConstraint_IsNull", 802, 1, 0x0016, 2021},
    {"AngleOfViewConstraint_IsNull", 803, 1, 0x0016, 2022},
    {"SafeZone_IsNull", 804, 1, 0x0016, 2023},
    {"PlayerSafeZone_IsNull", 805, 1, 0x0016, 2024},
    {"TrackToFrameTargets_IsNull", 807, 1, 0x0016, 2026},
    {"FreeLook_IsNull", 809, 1, 0x0016, 2028},
    {"MultipleTargets_IsNull", 811, 1, 0x0016, 2030},
    {"DollyObscuredBoss_IsNull", 813, 1, 0x0016, 2032},
    {"DollyObscuredPlayer_IsNull", 815, 1, 0x0016, 2034},
    {"TrackToMinimumDistance_IsNull", 817, 1, 0x0016, 2036},
    {"RailDrivesAnimation_IsNull", 819, 1, 0x0016, 2038},
    {"MoveDolly_IsNull", 821, 1, 0x0016, 2040},
    {"RailRelative_IsNull", 823, 1, 0x0016, 2042},
    {"JumpCompensation_IsNull", 825, 1, 0x0016, 2044},
    {"DoubleJumpComp_IsNull", 827, 1, 0x0016, 2046},
    {"UseStaticTarget_IsNull", 829, 1, 0x0016, 2048},
    {"IgnoreMicMaxDistance_IsNull", 831, 1, 0x0016, 2050},
    {"OverrideMicBoomRatio_IsNull", 833, 1, 0x0016, 2052},
    {"MicIsUnderwater_IsNull", 835, 1, 0x0016, 2054},
    {"CamIsUnderwater_IsNull", 837, 1, 0x0016, 2056},
    {"UsePlayerSafeZone_IsNull", 839, 1, 0x0016, 2058},
    {"Elevation_IsNull", 841, 1, 0x0016, 2059},
    {"Rotation_IsNull", 843, 1, 0x0016, 2060},
    {"DollyDamping_IsNull", 844, 1, 0x0016, 2061},
    {"JumpCompensationFactor_IsNull", 845, 1, 0x0016, 2062},
    {"MicBoomRatio_IsNull", 846, 1, 0x0016, 2063},
    {"DollyStartDefault_IsNull", 847, 1, 0x0016, 2064},
    {"StaticTarget_IsNull", 848, 1, 0x0016, 2065},
    {"ObjectTarget_IsNull", 849, 1, 0x0016, 2066},
    {"OrbitTarget_IsNull", 850, 1, 0x0016, 2067},
    {"CollisionTarget_IsNull", 851, 1, 0x0016, 2068},
    {"FightLineAngle_IsNull", 852, 1, 0x0016, 2069},
    {"CenterTargets_IsNull", 853, 1, 0x0016, 2070},
    {"RailFadeDirection_IsNull", 855, 1, 0x0016, 2071},
    {"RailFadeShelf_IsNull", 856, 1, 0x0016, 2072},
    {"RailFadeFalloff_IsNull", 857, 1, 0x0016, 2073},
    {"RailFadeEaseIn_IsNull", 858, 1, 0x0016, 2074},
    {"RailFadeEaseOut_IsNull", 859, 1, 0x0016, 2075},
    {"Curve_IsNull", 860, 1, 0x0016, 2076},
    {"DriveRail_IsNull", 861, 1, 0x0016, 2077},
    {"LeftStick_IsNull", 863, 1, 0x0016, 2078},
    {"RightStick_IsNull", 865, 1, 0x0016, 2079},
    {"LookFlags_IsNull", 867, 1, 0x0016, 2080},
    {"LookConstraintUp_IsNull", 868, 1, 0x0016, 2081},
    {"LookConstraintDown_IsNull", 869, 1, 0x0016, 2082},
    {"LookConstraintLeft_IsNull", 870, 1, 0x0016, 2083},
    {"LookConstraintRight_IsNull", 871, 1, 0x0016, 2084},
    {"MoveConstraintUp_IsNull", 872, 1, 0x0016, 2085},
    {"MoveConstraintDown_IsNull", 873, 1, 0x0016, 2086},
    {"MoveConstraintLeft_IsNull", 874, 1, 0x0016, 2087},
    {"MoveConstraintRight_IsNull", 875, 1, 0x0016, 2088},
    {"ElevateToFrameMax_IsNull", 876, 1, 0x0016, 2089},
    {"ElevateToFrameMin_IsNull", 877, 1, 0x0016, 2090},
    {"PlayerFrame_IsNull", 878, 1, 0x0016, 2091},
    {"TargetFrame_IsNull", 879, 1, 0x0016, 2092},
    {"TargetFrameDamping_IsNull", 880, 1, 0x0016, 2093},
    {"Follow_IsNull", 881, 1, 0x0016, 2094},
    {"SlopeFactor_IsNull", 882, 1, 0x0016, 2095},
    {"SlopeDamping_IsNull", 883, 1, 0x0016, 2096},
    {"GroundInfluence_IsNull", 884, 1, 0x0016, 2097},
    {"FrameTargets_IsNull", 886, 1, 0x0016, 2099},
    {"RotateToTargets_IsNull", 888, 1, 0x0016, 2101},
    {"TiltFrame_IsNull", 889, 1, 0x0016, 2102},
    {"TiltFrameMode_IsNull", 891, 1, 0x0016, 2103},
    {"PanLeft_IsNull", 892, 1, 0x0016, 2104},
    {"PanRight_IsNull", 893, 1, 0x0016, 2105},
    {"TiltUp_IsNull", 894, 1, 0x0016, 2106},
    {"TiltDown_IsNull", 895, 1, 0x0016, 2107},
    {"Distance_IsNull", 896, 1, 0x0016, 2108},
    {"PitchAdjust_IsNull", 897, 1, 0x0016, 2109},
    {"StrafeAssistFrame_IsNull", 898, 1, 0x0016, 2110},
    {"RotateToRail_IsNull", 899, 1, 0x0016, 2111},
};

inline constexpr Field kFields_03FB[] = {
    {"CameraType_IsNull", 711, 1, 0x0016, 2113},
    {"ParentRelative_IsNull", 713, 1, 0x0016, 2115},
    {"Aggression_IsNull", 715, 1, 0x0016, 2117},
    {"TargetActivated_IsNull", 717, 1, 0x0016, 2119},
    {"Collision_IsNull", 719, 1, 0x0016, 2121},
    {"AvoidEverything_IsNull", 721, 1, 0x0016, 2123},
    {"DontCheckpoint_IsNull", 723, 1, 0x0016, 2125},
    {"StartCentered_IsNull", 725, 1, 0x0016, 2127},
    {"RegisterName_IsNull", 727, 1, 0x0016, 2129},
    {"PhotoSensitive_IsNull", 729, 1, 0x0016, 2131},
    {"ConstraintFriction_IsNull", 731, 1, 0x0016, 2132},
    {"Position_IsNull", 732, 1, 0x0016, 2133},
    {"Yaw_IsNull", 733, 1, 0x0016, 2134},
    {"Pitch_IsNull", 734, 1, 0x0016, 2135},
    {"Roll_IsNull", 735, 1, 0x0016, 2136},
    {"focalLength_IsNull", 736, 1, 0x0016, 2137},
    {"AngleOfView_IsNull", 737, 1, 0x0016, 2138},
    {"FocusDistance_IsNull", 738, 1, 0x0016, 2139},
    {"FStop_IsNull", 739, 1, 0x0016, 2140},
    {"LensDistortion_IsNull", 740, 1, 0x0016, 2141},
    {"DepthOfField_IsNull", 742, 1, 0x0016, 2143},
    {"IsOrthographic_IsNull", 744, 1, 0x0016, 2145},
    {"OrthoWidth_IsNull", 745, 1, 0x0016, 2146},
    {"StateFilter_IsNull", 746, 1, 0x0016, 2147},
    {"RequireMarker_IsNull", 747, 1, 0x0016, 2148},
    {"IgnoreMarker_IsNull", 748, 1, 0x0016, 2149},
    {"DefaultTweenTime_IsNull", 749, 1, 0x0016, 2150},
    {"DefaultTweenDistance_IsNull", 750, 1, 0x0016, 2151},
    {"DefaultEaseIn_IsNull", 751, 1, 0x0016, 2152},
    {"DefaultEaseOut_IsNull", 752, 1, 0x0016, 2153},
    {"DefaultLengthIn_IsNull", 753, 1, 0x0016, 2154},
    {"DefaultLengthOut_IsNull", 754, 1, 0x0016, 2155},
    {"TweenOverrides_IsNull", 755, 1, 0x0016, 2156},
    {"Transition_IsNull", 756, 1, 0x0016, 2157},
    {"SplineTarget_IsNull", 757, 1, 0x0016, 2158},
    {"TargetCap_IsNull", 758, 1, 0x0016, 2159},
    {"MinimumTargetPriority_IsNull", 759, 1, 0x0016, 2160},
    {"MinimumTargetActivatedPriority_IsNull", 760, 1, 0x0016, 2161},
    {"TargetMatches_IsNull", 761, 1, 0x0016, 2162},
    {"StrafeAssistTargetMatches_IsNull", 762, 1, 0x0016, 2163},
    {"TargetActivatedMatches_IsNull", 763, 1, 0x0016, 2164},
    {"RotateToTargetMatches_IsNull", 764, 1, 0x0016, 2165},
    {"TiltTargetMatches_IsNull", 765, 1, 0x0016, 2166},
    {"TargetFilter_IsNull", 767, 1, 0x0016, 2167},
    {"BoomDamping_IsNull", 768, 1, 0x0016, 2168},
    {"VerticalDamping_IsNull", 769, 1, 0x0016, 2169},
    {"HorizontalDamping_IsNull", 770, 1, 0x0016, 2170},
    {"DampingForward_IsNull", 771, 1, 0x0016, 2171},
    {"DampingBackward_IsNull", 772, 1, 0x0016, 2172},
    {"DampingLeft_IsNull", 773, 1, 0x0016, 2173},
    {"DampingRight_IsNull", 774, 1, 0x0016, 2174},
    {"DampingUp_IsNull", 775, 1, 0x0016, 2175},
    {"DampingDown_IsNull", 776, 1, 0x0016, 2176},
    {"MinVelocity_IsNull", 777, 1, 0x0016, 2177},
    {"MaxVelocity_IsNull", 778, 1, 0x0016, 2178},
    {"VelocityEaseIn_IsNull", 779, 1, 0x0016, 2179},
    {"VelocityEaseOut_IsNull", 780, 1, 0x0016, 2180},
    {"VelocityDampingTime_IsNull", 781, 1, 0x0016, 2181},
    {"Effect_IsNull", 782, 1, 0x0016, 2182},
    {"OrbitControl_IsNull", 783, 1, 0x0016, 2183},
    {"TiltControl_IsNull", 784, 1, 0x0016, 2184},
    {"OrbitConstraint_IsNull", 785, 1, 0x0016, 2185},
    {"TiltConstraint_IsNull", 786, 1, 0x0016, 2186},
    {"Recenter_IsNull", 787, 1, 0x0016, 2187},
    {"AutoRecenter_IsNull", 788, 1, 0x0016, 2188},
    {"TiltRecenter_IsNull", 789, 1, 0x0016, 2189},
    {"Animation_IsNull", 790, 1, 0x0016, 2190},
    {"AnimationRate_IsNull", 791, 1, 0x0016, 2191},
    {"TriggerAnimation_IsNull", 793, 1, 0x0016, 2193},
    {"ForceAnimation_IsNull", 795, 1, 0x0016, 2195},
    {"BoomRatio_IsNull", 796, 1, 0x0016, 2196},
    {"MaxDistanceToDolly_IsNull", 797, 1, 0x0016, 2197},
    {"MinDistanceToDolly_IsNull", 798, 1, 0x0016, 2198},
    {"MaxDistanceToTarget_IsNull", 799, 1, 0x0016, 2199},
    {"MinDistanceToTarget_IsNull", 800, 1, 0x0016, 2200},
    {"ElevationConstraint_IsNull", 801, 1, 0x0016, 2201},
    {"RotationConstraint_IsNull", 802, 1, 0x0016, 2202},
    {"AngleOfViewConstraint_IsNull", 803, 1, 0x0016, 2203},
    {"SafeZone_IsNull", 804, 1, 0x0016, 2204},
    {"PlayerSafeZone_IsNull", 805, 1, 0x0016, 2205},
    {"TrackToFrameTargets_IsNull", 807, 1, 0x0016, 2207},
    {"FreeLook_IsNull", 809, 1, 0x0016, 2209},
    {"MultipleTargets_IsNull", 811, 1, 0x0016, 2211},
    {"DollyObscuredBoss_IsNull", 813, 1, 0x0016, 2213},
    {"DollyObscuredPlayer_IsNull", 815, 1, 0x0016, 2215},
    {"TrackToMinimumDistance_IsNull", 817, 1, 0x0016, 2217},
    {"RailDrivesAnimation_IsNull", 819, 1, 0x0016, 2219},
    {"MoveDolly_IsNull", 821, 1, 0x0016, 2221},
    {"RailRelative_IsNull", 823, 1, 0x0016, 2223},
    {"JumpCompensation_IsNull", 825, 1, 0x0016, 2225},
    {"DoubleJumpComp_IsNull", 827, 1, 0x0016, 2227},
    {"UseStaticTarget_IsNull", 829, 1, 0x0016, 2229},
    {"IgnoreMicMaxDistance_IsNull", 831, 1, 0x0016, 2231},
    {"OverrideMicBoomRatio_IsNull", 833, 1, 0x0016, 2233},
    {"MicIsUnderwater_IsNull", 835, 1, 0x0016, 2235},
    {"CamIsUnderwater_IsNull", 837, 1, 0x0016, 2237},
    {"UsePlayerSafeZone_IsNull", 839, 1, 0x0016, 2239},
    {"Elevation_IsNull", 841, 1, 0x0016, 2240},
    {"Rotation_IsNull", 843, 1, 0x0016, 2241},
    {"DollyDamping_IsNull", 844, 1, 0x0016, 2242},
    {"JumpCompensationFactor_IsNull", 845, 1, 0x0016, 2243},
    {"MicBoomRatio_IsNull", 846, 1, 0x0016, 2244},
    {"DollyStartDefault_IsNull", 847, 1, 0x0016, 2245},
    {"StaticTarget_IsNull", 848, 1, 0x0016, 2246},
    {"ObjectTarget_IsNull", 849, 1, 0x0016, 2247},
    {"OrbitTarget_IsNull", 850, 1, 0x0016, 2248},
    {"CollisionTarget_IsNull", 851, 1, 0x0016, 2249},
    {"FightLineAngle_IsNull", 852, 1, 0x0016, 2250},
    {"CenterTargets_IsNull", 853, 1, 0x0016, 2251},
    {"RailFadeDirection_IsNull", 855, 1, 0x0016, 2252},
    {"RailFadeShelf_IsNull", 856, 1, 0x0016, 2253},
    {"RailFadeFalloff_IsNull", 857, 1, 0x0016, 2254},
    {"RailFadeEaseIn_IsNull", 858, 1, 0x0016, 2255},
    {"RailFadeEaseOut_IsNull", 859, 1, 0x0016, 2256},
    {"Curve_IsNull", 860, 1, 0x0016, 2257},
    {"DriveRail_IsNull", 861, 1, 0x0016, 2258},
    {"LeftStick_IsNull", 863, 1, 0x0016, 2259},
    {"RightStick_IsNull", 865, 1, 0x0016, 2260},
    {"LookFlags_IsNull", 867, 1, 0x0016, 2261},
    {"LookConstraintUp_IsNull", 868, 1, 0x0016, 2262},
    {"LookConstraintDown_IsNull", 869, 1, 0x0016, 2263},
    {"LookConstraintLeft_IsNull", 870, 1, 0x0016, 2264},
    {"LookConstraintRight_IsNull", 871, 1, 0x0016, 2265},
    {"MoveConstraintUp_IsNull", 872, 1, 0x0016, 2266},
    {"MoveConstraintDown_IsNull", 873, 1, 0x0016, 2267},
    {"MoveConstraintLeft_IsNull", 874, 1, 0x0016, 2268},
    {"MoveConstraintRight_IsNull", 875, 1, 0x0016, 2269},
    {"ElevateToFrameMax_IsNull", 876, 1, 0x0016, 2270},
    {"ElevateToFrameMin_IsNull", 877, 1, 0x0016, 2271},
    {"PlayerFrame_IsNull", 878, 1, 0x0016, 2272},
    {"TargetFrame_IsNull", 879, 1, 0x0016, 2273},
    {"TargetFrameDamping_IsNull", 880, 1, 0x0016, 2274},
    {"Follow_IsNull", 881, 1, 0x0016, 2275},
    {"SlopeFactor_IsNull", 882, 1, 0x0016, 2276},
    {"SlopeDamping_IsNull", 883, 1, 0x0016, 2277},
    {"GroundInfluence_IsNull", 884, 1, 0x0016, 2278},
    {"FrameTargets_IsNull", 886, 1, 0x0016, 2280},
    {"RotateToTargets_IsNull", 888, 1, 0x0016, 2282},
    {"TiltFrame_IsNull", 889, 1, 0x0016, 2283},
    {"TiltFrameMode_IsNull", 891, 1, 0x0016, 2284},
    {"PanLeft_IsNull", 892, 1, 0x0016, 2285},
    {"PanRight_IsNull", 893, 1, 0x0016, 2286},
    {"TiltUp_IsNull", 894, 1, 0x0016, 2287},
    {"TiltDown_IsNull", 895, 1, 0x0016, 2288},
    {"Distance_IsNull", 896, 1, 0x0016, 2289},
    {"PitchAdjust_IsNull", 897, 1, 0x0016, 2290},
    {"StrafeAssistFrame_IsNull", 898, 1, 0x0016, 2291},
    {"RotateToRail_IsNull", 899, 1, 0x0016, 2292},
};

inline constexpr Field kFields_03FC[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"When", 8, 8, 0x0030, 65535},
    {"Marker", 16, 12, 0x0024, 457},
    {"DynamicFlag", 32, 12, 0x0024, 458},
    {"Zone", 48, 12, 0x0024, 459},
    {"Camera", 64, 12, 0x0024, 460},
};

inline constexpr Field kFields_03FD[] = {
    {"Rule", 0, 12, 0x0024, 461},
};

inline constexpr Field kFields_03FE[] = {
    {"TemplateSymbol", 0, 8, 0x001A, 0},
    {"Ceiling", 8, 4, 0x0008, 9302},
    {"Floor", 12, 4, 0x0008, 9303},
    {"Tether", 16, 4, 0x0008, 9304},
    {"WorldColliderRadius", 20, 4, 0x0008, 9305},
    {"LargeColliderRadius", 24, 4, 0x0008, 9306},
    {"Ceiling_IsNull", 28, 1, 0x0016, 2293},
    {"Floor_IsNull", 29, 1, 0x0016, 2294},
    {"Tether_IsNull", 30, 1, 0x0016, 2295},
    {"PhotoSensitive", 31, 1, 0x0014, 2296},
    {"PhotoSensitive_IsNull", 32, 1, 0x0016, 2297},
    {"OrbitConstraint", 33, 1, 0x0014, 2298},
    {"OrbitConstraint_IsNull", 34, 1, 0x0016, 2299},
    {"LargeCollider", 35, 1, 0x0014, 2300},
    {"LargeCollider_IsNull", 36, 1, 0x0016, 2301},
    {"WorldColliderRadius_IsNull", 37, 1, 0x0016, 2302},
    {"LargeColliderRadius_IsNull", 38, 1, 0x0016, 2303},
};

inline constexpr Field kFields_03FF[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"When", 8, 8, 0x0030, 65535},
    {"Marker", 16, 12, 0x0024, 462},
    {"DynamicFlag", 32, 12, 0x0024, 463},
    {"Zone", 48, 12, 0x0024, 464},
    {"Level", 64, 12, 0x0024, 465},
    {"Hack", 80, 12, 0x0024, 466},
};

inline constexpr Field kFields_0400[] = {
    {"Rule", 0, 12, 0x0024, 467},
    {"LargeColliderCreature", 16, 12, 0x0024, 468},
};

inline constexpr Field kFields_0401[] = {
    {"CameraRollScale", 0, 4, 0x0008, 9307},
    {"CameraRollSpringStrength", 4, 4, 0x0008, 9308},
    {"CameraRollDampingRatio", 8, 4, 0x0008, 9309},
    {"CameraRollMaxError", 12, 4, 0x0008, 9310},
    {"CameraPitchSpringStrength", 16, 4, 0x0008, 9311},
    {"CameraPitchDampingRatio", 20, 4, 0x0008, 9312},
    {"CameraPitchMaxError", 24, 4, 0x0008, 9313},
    {"CameraYawSpringStrength", 28, 4, 0x0008, 9314},
    {"CameraYawDampingRatio", 32, 4, 0x0008, 9315},
    {"CameraYawMaxError", 36, 4, 0x0008, 9316},
    {"CameraTranslationStiffness", 40, 4, 0x0008, 9317},
    {"CameraTranslationDampingRatio", 44, 4, 0x0008, 9318},
    {"MaxCameraDisplacement", 48, 4, 0x0008, 9319},
    {"CameraFOVSpringStrength", 52, 4, 0x0008, 9320},
    {"CameraFOVDampingRatio", 56, 4, 0x0008, 9321},
    {"CameraFOVMaxError", 60, 4, 0x0008, 9322},
    {"EnableCameraRollSmoothing", 64, 1, 0x0014, 2304},
    {"EnableCameraPitchSmoothing", 65, 1, 0x0014, 2305},
    {"EnableCameraYawSmoothing", 66, 1, 0x0014, 2306},
    {"EnableTranslationSmoothing", 67, 1, 0x0014, 2307},
    {"EnableCameraFOVSmoothing", 68, 1, 0x0014, 2308},
};

inline constexpr Field kFields_0402[] = {
    {"Cinematics", 0, 0, 0x0028, 30},
};

inline constexpr Field kFields_0403[] = {
    {"EntitlementIds", 0, 12, 0x0024, 469},
};

inline constexpr Field kFields_0404[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"DisplayUi", 8, 1, 0x0014, 2309},
    {"EnableComponents", 9, 1, 0x0014, 2310},
    {"SaveOption", 10, 1, 0x0104, 3030},
};

inline constexpr Field kFields_0405[] = {
    {"Wallets", 0, 12, 0x0024, 470},
};

inline constexpr Field kFields_0406[] = {
    {"ApplyTo", 0, 1, 0x0104, 3031},
    {"Type", 1, 1, 0x0105, 3032},
};

inline constexpr Field kFields_0407[] = {
    {"Flags", 0, 12, 0x0024, 471},
    {"LamsName", 12, 4, 0x0000, 1141},
    {"Components", 16, 12, 0x0024, 472},
    {"LamsDescription", 28, 4, 0x0000, 1142},
    {"Regions", 32, 12, 0x0024, 473},
    {"Max", 44, 4, 0x0000, 1143},
    {"NameId", 48, 8, 0x0018, 0},
    {"IconName", 56, 8, 0x0018, 0},
    {"DisplayUi", 64, 1, 0x0014, 2311},
    {"UpdateQuests", 65, 1, 0x0014, 2312},
    {"UpdateVisualScripts", 66, 1, 0x0014, 2313},
};

inline constexpr Field kFields_0408[] = {
    {"Flags", 0, 12, 0x0024, 474},
    {"Type", 12, 1, 0x0104, 3033},
    {"Wallet", 16, 8, 0x0010, 717},
    {"Label", 24, 8, 0x0018, 0},
    {"Resource", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_0409[] = {
    {"Flags", 0, 12, 0x0024, 475},
    {"DefaultAmount", 12, 4, 0x0000, 1144},
    {"Amount", 16, 12, 0x0024, 476},
    {"ReplacementResource", 32, 8, 0x0010, 0},
    {"NeverReplaceFlag", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_040A[] = {
    {"Resources", 0, 12, 0x0024, 477},
    {"ResourceIds", 16, 0, 0x0028, 31},
    {"FlagIds", 32, 12, 0x0024, 478},
    {"Telemetry", 48, 12, 0x0024, 479},
    {"ReplacementOptions", 64, 0, 0x002C, 1033},
};

inline constexpr Field kFields_040B[] = {
    {"ResourceName", 0, 8, 0x0010, 0},
    {"MinLevel", 8, 4, 0x0000, 1149},
    {"MaxLevel", 12, 4, 0x0000, 1150},
};

inline constexpr Field kFields_040C[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Amount", 8, 4, 0x0000, 1151},
    {"SortOrder", 12, 2, 0x0000, 1152},
    {"Type", 14, 1, 0x0104, 3034},
    {"Consume", 15, 1, 0x0014, 2314},
};

inline constexpr Field kFields_040D[] = {
    {"Flags", 0, 12, 0x0024, 482},
    {"LamsName", 12, 4, 0x0000, 1153},
    {"Input", 16, 12, 0x0024, 483},
    {"LamsDescription", 28, 4, 0x0000, 1154},
    {"Output", 32, 12, 0x0024, 484},
    {"CodeFlags", 44, 1, 0x0204, 3035},
    {"NameId", 48, 8, 0x0018, 0},
};

inline constexpr Field kFields_040E[] = {
    {"Recipes", 0, 12, 0x0024, 485},
    {"RecipeIds", 16, 0, 0x0028, 32},
};

inline constexpr Field kFields_040F[] = {
    {"AttributeName", 8, 8, 0x0010, 0},
    {"Value", 16, 4, 0x0008, 9323},
};

inline constexpr Field kFields_0410[] = {
    {"ResourceName", 8, 8, 0x0010, 0},
    {"Threshold", 16, 4, 0x0000, 1156},
    {"Amount", 20, 4, 0x0000, 1157},
};

inline constexpr Field kFields_0411[] = {
    {"MeterName", 8, 8, 0x0010, 0},
    {"Value", 16, 4, 0x0008, 9324},
};

inline constexpr Field kFields_0413[] = {
    {"Operand1", 0, 1, 0x0104, 3042},
    {"Operator", 1, 1, 0x0104, 3043},
    {"Operand2", 4, 4, 0x0008, 9325},
};

inline constexpr Field kFields_0414[] = {
    {"Type", 0, 1, 0x0104, 3044},
    {"Chance", 4, 4, 0x0008, 9326},
    {"Decision", 8, 12, 0x0024, 486},
};

inline constexpr Field kFields_0415[] = {
    {"FightAggression", 24, 0, 0x002C, 229},
    {"ProjectileAggression", 64, 0, 0x002C, 229},
    {"RecoveryTime", 104, 0, 0x002C, 229},
    {"Cooldown", 144, 0, 0x002C, 229},
    {"GlobalMoveRecoveryTime", 184, 0, 0x002C, 229},
    {"ID", 224, 8, 0x0010, 0},
    {"MinRange", 232, 2, 0x0008, 9358},
    {"MaxRange", 234, 2, 0x0008, 9359},
    {"InterruptBonusTime", 236, 2, 0x0008, 9360},
    {"Priority", 238, 1, 0x0000, 1158},
};

inline constexpr Field kFields_0416[] = {
    {"Move", 240, 8, 0x001C, 1286},
    {"DelayMinRange", 248, 4, 0x0008, 9395},
    {"DelayMaxRange", 252, 4, 0x0008, 9396},
};

inline constexpr Field kFields_0417[] = {
    {"Move", 24, 8, 0x001C, 1286},
};

inline constexpr Field kFields_0418[] = {
    {"Type", 0, 1, 0x0104, 3078},
};

inline constexpr Field kFields_0419[] = {
    {"ActionList", 8, 12, 0x0024, 500},
    {"BlockList", 24, 12, 0x0024, 501},
};

inline constexpr Field kFields_041A[] = {
    {"EntityName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_041B[] = {
    {"ZeroJointOffset", 8, 0, 0x002C, 6},
    {"EndPosOffset", 14, 0, 0x002C, 6},
    {"EntityType", 20, 4, 0x0204, 3084},
    {"CollidesWith", 24, 4, 0x0204, 3085},
    {"Angle", 28, 4, 0x0008, 9406},
    {"RaycastDistance", 32, 4, 0x0008, 9407},
    {"Type", 36, 1, 0x0104, 3086},
    {"OnlyRunOnRoundRobin", 37, 1, 0x0014, 2315},
    {"CheckNavmesh", 38, 1, 0x0014, 2316},
    {"NavmeshCheckFlags", 39, 1, 0x0204, 3087},
};

inline constexpr Field kFields_041C[] = {
    {"When", 8, 8, 0x0030, 65535},
};

inline constexpr Field kFields_041D[] = {
    {"DistanceToGround", 8, 4, 0x0008, 9410},
};

inline constexpr Field kFields_041E[] = {
    {"AttributeStatus", 8, 8, 0x001C, 1280},
};

inline constexpr Field kFields_041F[] = {
    {"NumberRequired", 8, 2, 0x0000, 1160},
    {"GameObjectName", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0420[] = {
    {"EnemyID", 8, 12, 0x0024, 502},
    {"CheckBlockedHit", 20, 1, 0x0014, 2317},
    {"EnemyContext", 24, 8, 0x0010, 0},
    {"ExposedSideName", 32, 8, 0x0010, 0},
    {"EnemyDynamicFlags", 40, 8, 0x0010, 0},
    {"HitFlags", 48, 8, 0x0204, 3098},
    {"AttackerPartFlags", 56, 8, 0x0204, 3099},
    {"DefenderPartFlags", 64, 8, 0x0204, 3100},
};

inline constexpr Field kFields_0421[] = {
    {"Target", 8, 1, 0x0104, 3103},
};

inline constexpr Field kFields_0422[] = {
    {"JointName", 8, 8, 0x0010, 0},
    {"SecondsOnScreen", 16, 4, 0x0008, 9416},
    {"PercentOnScreenLR", 20, 4, 0x0008, 9417},
    {"PercentOnScreenTB", 24, 4, 0x0008, 9418},
};

inline constexpr Field kFields_0423[] = {
    {"ID", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0424[] = {
    {"Marker", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0425[] = {
    {"Marker", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0426[] = {
    {"DynamicFlag", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0427[] = {
    {"DynamicFlag", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0428[] = {
    {"Context", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_042A[] = {
    {"PickupStatus", 16, 12, 0x0024, 503},
    {"PickupOperator", 28, 1, 0x0104, 3128},
};

inline constexpr Field kFields_042B[] = {
    {"PickupStatus", 16, 12, 0x0024, 504},
    {"PickupOperator", 28, 1, 0x0104, 3132},
};

inline constexpr Field kFields_042C[] = {
    {"SlotStatus", 16, 12, 0x0024, 505},
    {"SlotOperator", 28, 1, 0x0104, 3136},
};

inline constexpr Field kFields_042D[] = {
    {"SlotStatus", 16, 12, 0x0024, 506},
    {"SlotOperator", 28, 1, 0x0104, 3140},
};

inline constexpr Field kFields_042E[] = {
    {"ResourceStatus", 8, 0, 0x002C, 1035},
};

inline constexpr Field kFields_042F[] = {
    {"Tags", 8, 4, 0x0204, 3145},
};

inline constexpr Field kFields_0430[] = {
    {"Tags", 12, 4, 0x0204, 3149},
};

inline constexpr Field kFields_0431[] = {
    {"MoveName", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0432[] = {
    {"ZoneGOName", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0433[] = {
    {"EventTypeName", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0434[] = {
    {"VariableName", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0435[] = {
    {"DriverName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0436[] = {
    {"Name", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0437[] = {
    {"FilterData", 8, 0, 0x002C, 247},
    {"UsePlayerForCheck", 104, 1, 0x0014, 2318},
    {"BlackboardOperand2", 105, 1, 0x0104, 3169},
    {"BlackboardEntryName", 112, 8, 0x0010, 0},
};

inline constexpr Field kFields_0438[] = {
    {"ReferenceAxis", 8, 1, 0x0104, 3172},
    {"PlaneNormal", 9, 1, 0x0104, 3173},
    {"Angle", 10, 0, 0x002C, 43},
};

inline constexpr Field kFields_043A[] = {
    {"Elevation", 16, 0, 0x002C, 43},
    {"Depth", 20, 0, 0x002C, 43},
    {"Side", 24, 0, 0x002C, 43},
    {"EnvironmentSurfaceAngle", 28, 0, 0x002C, 43},
    {"TargetSheet", 32, 8, 0x0204, 3182},
    {"IgnoreSheet", 40, 8, 0x0204, 3183},
    {"Material", 48, 8, 0x0010, 0},
};

inline constexpr Field kFields_043C[] = {
    {"StateFilter", 8, 8, 0x001C, 152},
    {"LogicStateFilter", 16, 8, 0x001C, 153},
};

inline constexpr Field kFields_043D[] = {
    {"Components", 8, 12, 0x0024, 512},
};

inline constexpr Field kFields_043E[] = {
    {"Decision", 8, 8, 0x001C, 1043},
};

inline constexpr Field kFields_043F[] = {
    {"Decision", 8, 12, 0x0024, 513},
};

inline constexpr Field kFields_0440[] = {
    {"Decision", 8, 12, 0x0024, 514},
};

inline constexpr Field kFields_0441[] = {
    {"Time", 12, 4, 0x0008, 9466},
};

inline constexpr Field kFields_0442[] = {
    {"Decision", 8, 8, 0x001C, 1043},
    {"TrueNode", 16, 8, 0x001C, 1048},
    {"FalseNode", 24, 8, 0x001C, 1048},
};

inline constexpr Field kFields_0443[] = {
    {"Difficulty", 8, 0, 0x0028, 33},
};

inline constexpr Field kFields_0444[] = {
    {"Type", 0, 1, 0x0104, 3202},
    {"Multiplier", 4, 4, 0x0008, 9467},
};

inline constexpr Field kFields_0445[] = {
    {"Decision", 8, 12, 0x0024, 515},
    {"DTTFlags", 20, 1, 0x0204, 3204},
    {"pointer_8", 24, 8, 0x001C, 1043},
};

inline constexpr Field kFields_0446[] = {
    {"HookName", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0448[] = {
    {"Flag", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0449[] = {
    {"Root", 0, 8, 0x001C, 1048},
};

inline constexpr Field kFields_044A[] = {
    {"Flags", 2, 2, 0x0204, 3212},
};

inline constexpr Field kFields_044B[] = {
    {"BlockHitFlagsList", 8, 12, 0x0024, 516},
    {"FightStanceValues", 24, 8, 0x001C, 179},
    {"CameraTargetSet", 32, 12, 0x0024, 517},
};

inline constexpr Field kFields_044E[] = {
    {"Distance", 0, 4, 0x0008, 9472},
    {"Angle", 4, 4, 0x0008, 9473},
    {"AcceptedIDs", 8, 12, 0x0024, 518},
};

inline constexpr Field kFields_044F[] = {
    {"AggressiveStandardRecoveryTime", 0, 4, 0x0008, 9474},
    {"NonAggressiveStandardRecoveryTime", 4, 4, 0x0008, 9475},
    {"DecisionTree", 8, 8, 0x001C, 1097},
};

inline constexpr Field kFields_0450[] = {
    {"CombatAggroStateName", 0, 8, 0x0010, 0},
    {"NoCombatAITimeoutTimer", 8, 4, 0x0008, 9476},
    {"CombatAIOutOfRangeTimeoutTimer", 12, 4, 0x0008, 9477},
    {"CombatAIOutOfRangeDistance", 16, 4, 0x0008, 9478},
};

inline constexpr Field kFields_0451[] = {
    {"TargetPicker", 0, 12, 0x0024, 519},
    {"VisualRadius", 12, 4, 0x0008, 9479},
    {"VisualDegree", 16, 4, 0x0008, 9480},
    {"TargetPickerRefractoryPeriod", 20, 4, 0x0008, 9481},
};

inline constexpr Field kFields_0452[] = {
    {"VarianceSpeed", 0, 4, 0x0008, 9482},
    {"innerRadius", 4, 4, 0x0008, 9483},
    {"tracingDistance", 8, 4, 0x0008, 9484},
    {"outerRadius", 12, 4, 0x0008, 9485},
};

inline constexpr Field kFields_0453[] = {
    {"Tag", 0, 8, 0x0010, 0},
    {"Consumable", 8, 1, 0x0014, 2319},
    {"AttachedToSource", 9, 1, 0x0014, 2320},
    {"Duration", 12, 4, 0x0008, 9486},
    {"Radius", 16, 4, 0x0008, 9487},
    {"MinAnglePositionFromSourceForward", 20, 4, 0x0008, 9488},
    {"MaxAnglePositionFromSourceForward", 24, 4, 0x0008, 9489},
    {"Instant", 28, 1, 0x0014, 2321},
};

inline constexpr Field kFields_0454[] = {
    {"CollisionAvoidance", 0, 0, 0x002C, 1529},
    {"DistanceWeight", 12, 4, 0x0008, 9493},
    {"Points", 16, 12, 0x0024, 520},
    {"PriorityWeight", 28, 4, 0x0008, 9494},
    {"PointRings", 32, 12, 0x0024, 521},
    {"FacingWeight", 44, 4, 0x0008, 9495},
    {"Arcs", 48, 12, 0x0024, 522},
    {"RaycastFailureState", 60, 0, 0x002C, 1528},
    {"DebugShowWeights", 65, 1, 0x0104, 3220},
    {"DebugShowWeightDetail", 66, 1, 0x0104, 3221},
    {"PointFacingReference", 67, 1, 0x0104, 3222},
    {"FutureDistance", 68, 4, 0x0008, 9496},
    {"FutureTime", 72, 4, 0x0008, 9497},
    {"SwitchToInvalidGraceTime", 76, 4, 0x0008, 9498},
    {"PointPlacement", 80, 1, 0x0104, 3223},
    {"InvalidationTimerMode", 81, 1, 0x0104, 3224},
};

inline constexpr Field kFields_0455[] = {
    {"TheaterScale", 0, 4, 0x0008, 9499},
    {"OffCameraMultiplier", 4, 4, 0x0008, 9500},
    {"InCombatZoneMultiplier", 8, 4, 0x0008, 9501},
    {"PenetratingCombatZoneMultiplier", 12, 4, 0x0008, 9502},
    {"NearCombatZoneMultiplier", 16, 4, 0x0008, 9503},
    {"NearCombatZoneDistance", 20, 4, 0x0008, 9504},
    {"GoalFilterInternalRadius", 24, 4, 0x0008, 9505},
    {"GoalFilterExternalRadius", 28, 4, 0x0008, 9506},
    {"GoalFilterInternalMultiplier", 32, 4, 0x0008, 9507},
    {"GoalFilterRingMultiplier", 36, 4, 0x0008, 9508},
    {"GoalFilterExternalMultiplier", 40, 4, 0x0008, 9509},
    {"ClosestEnemyFilterInternalRadius", 44, 4, 0x0008, 9510},
    {"ClosestEnemyFilterExternalRadius", 48, 4, 0x0008, 9511},
    {"ClosestEnemyFilterInternalMultiplier", 52, 4, 0x0008, 9512},
    {"ClosestEnemyFilterRingMultiplier", 56, 4, 0x0008, 9513},
    {"ClosestEnemyFilterExternalMultiplier", 60, 4, 0x0008, 9514},
    {"InputTargetFilterInternalRadius", 64, 4, 0x0008, 9515},
    {"InputTargetFilterExternalRadius", 68, 4, 0x0008, 9516},
    {"InputTargetFilterInternalMultiplier", 72, 4, 0x0008, 9517},
    {"InputTargetFilterRingMultiplier", 76, 4, 0x0008, 9518},
    {"InputTargetFilterExternalMultiplier", 80, 4, 0x0008, 9519},
    {"CurrentTargetFilterInternalRadius", 84, 4, 0x0008, 9520},
    {"CurrentTargetFilterExternalRadius", 88, 4, 0x0008, 9521},
    {"CurrentTargetFilterInternalMultiplier", 92, 4, 0x0008, 9522},
    {"CurrentTargetFilterRingMultiplier", 96, 4, 0x0008, 9523},
    {"CurrentTargetFilterExternalMultiplier", 100, 4, 0x0008, 9524},
    {"OwnerFilterInternalRadius", 104, 4, 0x0008, 9525},
    {"OwnerFilterExternalRadius", 108, 4, 0x0008, 9526},
    {"OwnerFilterInternalMultiplier", 112, 4, 0x0008, 9527},
    {"OwnerFilterRingMultiplier", 116, 4, 0x0008, 9528},
    {"OwnerFilterExternalMultiplier", 120, 4, 0x0008, 9529},
    {"PlayerFilterInternalRadius", 124, 4, 0x0008, 9530},
    {"PlayerFilterExternalRadius", 128, 4, 0x0008, 9531},
    {"PlayerFilterInternalMultiplier", 132, 4, 0x0008, 9532},
    {"PlayerFilterRingMultiplier", 136, 4, 0x0008, 9533},
    {"PlayerFilterExternalMultiplier", 140, 4, 0x0008, 9534},
    {"AllyFilterInternalRadius", 144, 4, 0x0008, 9535},
    {"AllyFilterExternalRadius", 148, 4, 0x0008, 9536},
    {"AllyFilterInternalMultiplier", 152, 4, 0x0008, 9537},
    {"AllyFilterRingMultiplier", 156, 4, 0x0008, 9538},
    {"AllyFilterExternalMultiplier", 160, 4, 0x0008, 9539},
    {"HeroDirX", 164, 4, 0x0008, 9540},
    {"HeroDirZ", 168, 4, 0x0008, 9541},
    {"HeroDirFilterOffset", 172, 4, 0x0008, 9542},
    {"HeroDirFilterVariance", 176, 4, 0x0008, 9543},
    {"CameraDirX", 180, 4, 0x0008, 9544},
    {"CameraDirZ", 184, 4, 0x0008, 9545},
    {"CameraDirFilterOffset", 188, 4, 0x0008, 9546},
    {"CameraDirFilterVariance", 192, 4, 0x0008, 9547},
    {"PathingWeightMultiplier", 196, 4, 0x0008, 9548},
    {"EnemyLeadTime", 200, 4, 0x0008, 9549},
    {"HeroLeadTime", 204, 4, 0x0008, 9550},
    {"NearHeroDistance", 208, 4, 0x0008, 9551},
    {"NearEnemyDistance", 212, 4, 0x0008, 9552},
    {"ExcludeHostileDistance", 216, 4, 0x0008, 9553},
    {"StartPositionOverrideWeight", 220, 4, 0x0008, 9554},
    {"DebugNormalizeColors", 224, 1, 0x0014, 2322},
    {"UseOffCameraFilter", 225, 1, 0x0014, 2323},
    {"UseCombatZoneFilter", 226, 1, 0x0014, 2324},
    {"UseGoalDistanceFilter", 227, 1, 0x0014, 2325},
    {"UseClosestEnemyDistanceFilter", 228, 1, 0x0014, 2326},
    {"UseInputTargetDistanceFilter", 229, 1, 0x0014, 2327},
    {"UseCurrentTargetDistanceFilter", 230, 1, 0x0014, 2328},
    {"UseOwnerDistanceFilter", 231, 1, 0x0014, 2329},
    {"UsePlayerDistanceFilter", 232, 1, 0x0014, 2330},
    {"UseAllyDistanceFilter", 233, 1, 0x0014, 2331},
    {"UseHeroDirFilter", 234, 1, 0x0014, 2332},
    {"UseCameraDirFilter", 235, 1, 0x0014, 2333},
    {"IgnoreEnemyTiles", 236, 1, 0x0014, 2334},
    {"IgnoreHeroTile", 237, 1, 0x0014, 2335},
    {"Flags", 238, 1, 0x0204, 3225},
};

inline constexpr Field kFields_0456[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"LocalX", 8, 4, 0x0008, 9555},
    {"LocalY", 12, 4, 0x0008, 9556},
    {"LocalZ", 16, 4, 0x0008, 9557},
    {"Radius", 20, 4, 0x0008, 9558},
    {"Allow360Approach", 24, 1, 0x0014, 2336},
};

inline constexpr Field kFields_0457[] = {
    {"CABranchInfoList", 0, 12, 0x0024, 523},
};

inline constexpr Field kFields_0458[] = {
    {"Type", 0, 1, 0x0104, 3226},
    {"ID", 4, 4, 0x0000, 1163},
};

inline constexpr Field kFields_0459[] = {
    {"Start", 8, 4, 0x0008, 9559},
    {"End", 12, 4, 0x0008, 9560},
};

inline constexpr Field kFields_045A[] = {
    {"BaseType", 8, 1, 0x0104, 3229},
    {"BaseAxis", 9, 1, 0x0104, 3230},
    {"Origin", 10, 0, 0x002C, 6},
    {"Width", 16, 4, 0x0008, 9564},
    {"Depth", 20, 4, 0x0008, 9565},
    {"Part", 24, 12, 0x0024, 524},
};

inline constexpr Field kFields_045B[] = {
    {"Partition", 0, 0, 0x002C, 1114},
    {"Match", 40, 12, 0x0024, 526},
    {"ID", 52, 4, 0x0000, 1167},
    {"RootJoint", 56, 8, 0x0018, 718},
};

inline constexpr Field kFields_045C[] = {
    {"DefaultStance", 0, 8, 0x001C, 1098},
    {"StartupBrain", 8, 8, 0x001C, 1042},
    {"SpawnClearRadius", 16, 4, 0x0008, 9571},
    {"CullRadius", 20, 4, 0x0008, 9572},
    {"Type", 24, 1, 0x0105, 3234},
    {"SpawnCullingEnabled", 25, 1, 0x0014, 2337},
    {"SoundRegister", 26, 1, 0x0000, 1168},
    {"MemoryRequired", 27, 1, 0x0204, 3235},
    {"DebugFlags", 28, 1, 0x0204, 3236},
};

inline constexpr Field kFields_045D[] = {
    {"TargetPickerSearchRadius", 32, 4, 0x0008, 9575},
    {"TargetPickerRefractoryPeriod", 36, 4, 0x0008, 9576},
    {"TargetPicker", 40, 12, 0x0024, 527},
};

inline constexpr Field kFields_045E[] = {
    {"PassiveFlags", 56, 1, 0x0204, 3243},
    {"AutoUnfreeze", 60, 4, 0x0008, 9581},
    {"StartBranch", 64, 8, 0x001C, 1286},
};

inline constexpr Field kFields_0460[] = {
    {"VehicleControls", 32, 8, 0x001C, 1275},
    {"SlotList", 40, 12, 0x0024, 530},
    {"AttachmentList", 56, 12, 0x0024, 531},
    {"Meters", 72, 12, 0x0024, 532},
};

inline constexpr Field kFields_0461[] = {
    {"CloseToThrowerMaxDistance", 0, 4, 0x0008, 9589},
    {"WarnApproachMinAngle", 4, 4, 0x0008, 9590},
    {"WarnApproachTime", 8, 4, 0x0008, 9591},
    {"WarnVisionAngle", 12, 4, 0x0008, 9592},
    {"PredictImpactPoint", 16, 1, 0x0014, 2342},
};

inline constexpr Field kFields_0462[] = {
    {"BlockMoveList", 32, 12, 0x0024, 533},
    {"FadeDist", 44, 4, 0x0008, 9595},
    {"BlockContextList", 48, 12, 0x0024, 534},
    {"ProxRadius", 60, 4, 0x0008, 9596},
    {"ProjectileAwareness", 64, 8, 0x001C, 1121},
    {"ConsiderInFightDist", 72, 4, 0x0008, 9597},
    {"LeashRadius", 76, 4, 0x0008, 9598},
    {"WanderSpeed", 80, 4, 0x0008, 9599},
    {"ChaseSpeed", 84, 4, 0x0008, 9600},
    {"TimeTillLost", 88, 4, 0x0008, 9601},
    {"BehaviorTreeUpdateMinimumWait", 92, 4, 0x0008, 9602},
    {"UpdatePriority", 96, 4, 0x0008, 9603},
    {"HealthMeterIdx", 100, 1, 0x0000, 1174},
};

inline constexpr Field kFields_0463[] = {
    {"PropertiesPerForm", 0, 0, 0x0028, 34},
    {"MayaNames", 16, 0, 0x002C, 167},
};

inline constexpr Field kFields_0464[] = {
    {"ForwardDirectionalScaleSignedTargetSpeedList", 0, 12, 0x0024, 535},
    {"Mass", 12, 4, 0x0008, 9605},
    {"ForwardDirectionalScaleAngleList", 16, 12, 0x0024, 536},
    {"MaxSpeed", 28, 4, 0x0008, 9607},
    {"BackwardDirectionalScaleSignedTargetSpeedList", 32, 12, 0x0024, 537},
    {"MaxRotationSpeed", 44, 4, 0x0008, 9609},
    {"BackwardDirectionalScaleAngleList", 48, 12, 0x0024, 538},
    {"CenterOfMass", 60, 0, 0x002C, 7},
    {"LinearDamping", 72, 4, 0x0008, 9614},
    {"AngularDamping", 76, 4, 0x0008, 9615},
    {"LateralDamping", 80, 4, 0x0008, 9616},
    {"Acceleration", 84, 4, 0x0008, 9617},
    {"AngularAcceleration", 88, 4, 0x0008, 9618},
    {"BackwardScaling", 92, 4, 0x0008, 9619},
    {"MinRightStickControlAngle", 96, 4, 0x0008, 9620},
    {"MaxSpeedChangeRate", 100, 4, 0x0008, 9621},
    {"MaxRotationSpeedChangeRate", 104, 4, 0x0008, 9622},
    {"AccelerationChangeRate", 108, 4, 0x0008, 9623},
    {"AngularAccelerationChangeRate", 112, 4, 0x0008, 9624},
    {"CameraRotationWeight", 116, 4, 0x0008, 9625},
    {"EnvironmentRotationWeight", 120, 4, 0x0008, 9626},
    {"Flags", 124, 1, 0x0204, 3254},
};

inline constexpr Field kFields_0465[] = {
    {"InertiaClamp", 0, 4, 0x0008, 9627},
    {"InertiaTurnFactor", 4, 4, 0x0008, 9628},
    {"SledFrictionResetRate", 8, 4, 0x0008, 9629},
    {"TargetSledFrictionDuringDrift", 12, 4, 0x0008, 9630},
    {"TargetWolfFrictionDuringDrift", 16, 4, 0x0008, 9631},
};

inline constexpr Field kFields_0466[] = {
    {"CollisionRadius", 0, 4, 0x0008, 9632},
    {"MovementPredictionCastRadius", 4, 4, 0x0008, 9633},
    {"StopTestLength", 8, 4, 0x0008, 9634},
    {"RedirectTestLength", 12, 4, 0x0008, 9635},
    {"StopOrRedirectTestAngle", 16, 4, 0x0008, 9636},
    {"SlowdownTestLength", 20, 4, 0x0008, 9637},
    {"SlowdownTestAngle", 24, 4, 0x0008, 9638},
};

inline constexpr Field kFields_0467[] = {
    {"AngularSpeedExtremeTurnScale", 0, 4, 0x0008, 9639},
    {"LinearSpeedExtremeTurnScale", 4, 4, 0x0008, 9640},
    {"ExtremeTurnIntoDriftTransitionTime", 8, 4, 0x0008, 9641},
    {"TargetSledFriction", 12, 4, 0x0008, 9642},
    {"TargetWolfFriction", 16, 4, 0x0008, 9643},
    {"SledFrictionAcceleration", 20, 4, 0x0008, 9644},
    {"WolfFrictionAcceleration", 24, 4, 0x0008, 9645},
    {"SledFrictionResetRate", 28, 4, 0x0008, 9646},
    {"WolfFrictionResetRate", 32, 4, 0x0008, 9647},
    {"AdditionalInertiaFactor", 36, 4, 0x0008, 9648},
    {"InitialAdditionalInertiaFactor", 40, 4, 0x0008, 9649},
    {"InitialInertiaDuration", 44, 4, 0x0008, 9650},
    {"InitialInertiaMinAngularSpeedToUse", 48, 4, 0x0008, 9651},
    {"InertiaClamp", 52, 4, 0x0008, 9652},
    {"InertiaMinLinearSpeed", 56, 4, 0x0008, 9653},
    {"InertiaMaxLinearSpeed", 60, 4, 0x0008, 9654},
    {"InertiaLinearSpeedMultiplier", 64, 4, 0x0008, 9655},
    {"InertiaDecayOnTurn", 68, 4, 0x0008, 9656},
    {"InertiaDecayFullFriction_Multiply", 72, 4, 0x0008, 9657},
    {"InertiaDecayNoFriction_Multiply", 76, 4, 0x0008, 9658},
    {"InertiaDecay_Subtract", 80, 4, 0x0008, 9659},
    {"InertiaDecayAbruptStop", 84, 4, 0x0008, 9660},
    {"MaxControlAngleForMaxDecay", 88, 4, 0x0008, 9661},
    {"ControlAngleForDecayEndpoint", 92, 4, 0x0008, 9662},
    {"SidewaysAcceleration_NoFriction", 96, 4, 0x0008, 9663},
    {"SidewaysAcceleration_FullFriction", 100, 4, 0x0008, 9664},
    {"ForwardAcceleration_NoFriction", 104, 4, 0x0008, 9665},
    {"ForwardAcceleration_FullFriction", 108, 4, 0x0008, 9666},
    {"SpeedControlDeceleration", 112, 4, 0x0008, 9667},
    {"SpeedControlMaxDecelerationTime", 116, 4, 0x0008, 9668},
    {"WolvesOppositionForMaxFriction", 120, 4, 0x0008, 9669},
    {"WolvesOppositionForMinFriction", 124, 4, 0x0008, 9670},
    {"WolvesOppositionFrictionFactor", 128, 4, 0x0008, 9671},
    {"WolvesAccelerationDuringExtremeDrift", 132, 4, 0x0008, 9672},
    {"CounterSteerInertiaBoost", 136, 4, 0x0008, 9673},
    {"AnimDriverAcceleration", 140, 4, 0x0008, 9674},
    {"AnimDriverDecceleration", 144, 4, 0x0008, 9675},
    {"DriftDelayAfterReverseTurn", 148, 4, 0x0008, 9676},
    {"SummoningSledClampAngle1", 152, 4, 0x0008, 9677},
    {"SummoningSledClampAngle2", 156, 4, 0x0008, 9678},
    {"SummoningSledClampAngularAcceleration", 160, 4, 0x0008, 9679},
    {"GeneralClampAngle", 164, 4, 0x0008, 9680},
    {"GeneralClampFactor", 168, 4, 0x0008, 9681},
    {"GeneralClampAddsVelocity", 172, 1, 0x0014, 2344},
};

inline constexpr Field kFields_0468[] = {
    {"CastForwardOffset", 0, 4, 0x0008, 9682},
    {"CastUpwardOffset", 4, 4, 0x0008, 9683},
    {"CastLength", 8, 4, 0x0008, 9684},
    {"CastInnerRadius", 12, 4, 0x0008, 9685},
    {"CastOuterRadius", 16, 4, 0x0008, 9686},
    {"MaxExtractionSpeed", 20, 4, 0x0008, 9687},
    {"DriftDisableTime", 24, 4, 0x0008, 9688},
};

inline constexpr Field kFields_0469[] = {
    {"CylinderOffsetY", 0, 4, 0x0008, 9689},
    {"CylinderRadiusDuringFullReverse", 4, 4, 0x0008, 9690},
    {"CylinderLength", 8, 4, 0x0008, 9691},
    {"ExtractionVelocity", 12, 4, 0x0008, 9692},
    {"FallbackReverseStartAngle", 16, 4, 0x0008, 9693},
    {"FallbackReverseEndAngle", 20, 4, 0x0008, 9694},
};

inline constexpr Field kFields_046A[] = {
    {"MaxSpeedForPlayerExit", 0, 4, 0x0008, 9695},
    {"WolfAngularSpeedFromAnimationScale", 4, 4, 0x0008, 9696},
    {"WolfAngularSpeedFromAnimationScaleHighGear", 8, 4, 0x0008, 9697},
    {"WolfAngularSpeedFromAnimationScaleSummoning", 12, 4, 0x0008, 9698},
    {"WolfAngularSpeedExtremeTurnScale", 16, 4, 0x0008, 9699},
    {"WolfLinearSpeedExtremeTurnScale", 20, 4, 0x0008, 9700},
    {"WolfAngularSpeedExtremeTurnMinAngle", 24, 4, 0x0008, 9701},
    {"WolfAngularSpeedExtremeTurnMaxAngle", 28, 4, 0x0008, 9702},
};

inline constexpr Field kFields_046B[] = {
    {"LinearSpeedScale", 0, 4, 0x0008, 9703},
    {"AngularSpeedScale", 4, 4, 0x0008, 9704},
    {"ClampAngle", 8, 4, 0x0008, 9705},
    {"TransitionStartAngle", 12, 4, 0x0008, 9706},
    {"TransitionEndAngle", 16, 4, 0x0008, 9707},
    {"OppositeExitAngle", 20, 4, 0x0008, 9708},
    {"OppositeExitTransitionTime", 24, 4, 0x0008, 9709},
};

inline constexpr Field kFields_046C[] = {
    {"Collision", 0, 0, 0x002C, 1126},
    {"Drift", 28, 0, 0x002C, 1127},
    {"DriftCollisionReposition", 204, 0, 0x002C, 1128},
    {"ReverseCollisionReposition", 232, 0, 0x002C, 1129},
    {"General", 256, 0, 0x002C, 1130},
    {"Ice", 288, 0, 0x002C, 1125},
    {"ReverseTurn", 308, 0, 0x002C, 1131},
    {"TestHaptic", 336, 8, 0x001C, 488},
    {"DriftStartHaptic", 344, 8, 0x001C, 488},
    {"SledStopHaptic", 352, 8, 0x001C, 488},
    {"AlwaysPlayingHaptic", 360, 8, 0x001C, 481},
};

inline constexpr Field kFields_046D[] = {
    {"RotationSpeedNormal", 0, 4, 0x0008, 9793},
    {"RotationSpeedCollision", 4, 4, 0x0008, 9794},
    {"VerticalDampingFactor", 8, 4, 0x0008, 9795},
    {"SpeedModifier", 12, 4, 0x0008, 9796},
    {"MinSpringWeight", 16, 4, 0x0008, 9797},
    {"MaxSpringWeight", 20, 4, 0x0008, 9798},
    {"DistanceOfMaxSpringWeight", 24, 4, 0x0008, 9799},
    {"DampeningRatio", 28, 4, 0x0008, 9800},
    {"MaxDistanceFromSpline", 32, 4, 0x0008, 9801},
    {"MaxPitchAngle", 36, 4, 0x0008, 9802},
    {"PitchAdjustmentRange", 40, 4, 0x0008, 9803},
    {"PitchAdjustmentPercentage", 44, 4, 0x0008, 9804},
    {"MaxRollAngle", 48, 4, 0x0008, 9805},
    {"RollAdjustmentRange", 52, 4, 0x0008, 9806},
    {"RollAdjustmentPercentage", 56, 4, 0x0008, 9807},
    {"RollScalar", 60, 4, 0x0008, 9808},
    {"LookAheadPercentageAmount", 64, 4, 0x0008, 9809},
};

inline constexpr Field kFields_046E[] = {
    {"MaxSpeed", 0, 4, 0x0008, 9810},
    {"WaterSpeed", 4, 4, 0x0008, 9811},
    {"MaxWaterHeightCheck", 8, 4, 0x0008, 9812},
    {"MaxWaterHeightForBlend", 12, 4, 0x0008, 9813},
    {"CollisionCheckDistance", 16, 4, 0x0008, 9814},
    {"CollisionAutoStopDistance", 20, 4, 0x0008, 9815},
    {"CollisionRedirectionRate", 24, 4, 0x0008, 9816},
    {"CollisionStopAngle", 28, 4, 0x0008, 9817},
    {"CollisionStopAngleTraversal", 32, 4, 0x0008, 9818},
    {"StickIntentRunThreshold", 36, 4, 0x0008, 9819},
    {"StickIntentTrotThreshold", 40, 4, 0x0008, 9820},
    {"StickIntentWalkThreshold", 44, 4, 0x0008, 9821},
    {"StickIntentSpeedChangeDelay", 48, 4, 0x0008, 9822},
    {"StickIntentDeadZoneDelay", 52, 4, 0x0008, 9823},
    {"StickAngleForFullTurn", 56, 4, 0x0008, 9824},
    {"ReverseTurnMaxAngle", 60, 4, 0x0008, 9825},
    {"BackwardsStopAngle", 64, 4, 0x0008, 9826},
    {"CameraRelativeStartSpeed", 68, 4, 0x0008, 9827},
    {"CameraRelativeStickAngleMax", 72, 4, 0x0008, 9828},
    {"CameraRelativeTargetAngle", 76, 4, 0x0008, 9829},
    {"SlopeDampingFactor", 80, 4, 0x0008, 9830},
    {"AllowInteractMaxYakSpeed", 84, 4, 0x0008, 9831},
    {"UseCollisionAvoidance", 88, 1, 0x0014, 2346},
    {"AllowReverseTurnIntoWalls", 89, 1, 0x0014, 2347},
    {"UseCameraRelativeStart", 90, 1, 0x0014, 2348},
};

inline constexpr Field kFields_046F[] = {
    {"TypeName", 0, 8, 0x0010, 0},
    {"Priority", 8, 1, 0x0000, 1175},
};

inline constexpr Field kFields_0470[] = {
    {"GazeTimerMin", 0, 4, 0x0008, 9832},
    {"GazeTimerMax", 4, 4, 0x0008, 9833},
    {"GazeCooldownMin", 8, 4, 0x0008, 9834},
    {"GazeCooldownMax", 12, 4, 0x0008, 9835},
    {"GroupPerTargetTimerMin", 16, 4, 0x0008, 9836},
    {"GroupPerTargetTimerMax", 20, 4, 0x0008, 9837},
    {"RepositionDelayMin", 24, 4, 0x0008, 9838},
    {"RepositionDelayMax", 28, 4, 0x0008, 9839},
    {"CreaturePriorities", 32, 12, 0x0024, 539},
    {"SpeakerPriority", 48, 0, 0x002C, 1135},
    {"InterestingObjectPriority", 64, 0, 0x002C, 1135},
    {"LookAtStimPriority", 80, 0, 0x002C, 1135},
};

inline constexpr Field kFields_0471[] = {
    {"WeaponTrailJoints", 0, 12, 0x0024, 540},
    {"WeaponLeadingEdgeJoints", 16, 12, 0x0024, 541},
};

inline constexpr Field kFields_0472[] = {
    {"List", 0, 12, 0x0024, 542},
};

inline constexpr Field kFields_0473[] = {
    {"JointName", 0, 8, 0x0010, 0},
    {"EmbedMask", 8, 2, 0x0104, 3255},
};

inline constexpr Field kFields_0474[] = {
    {"WeaponName", 0, 8, 0x0010, 0},
    {"EmbedPoints", 8, 12, 0x0024, 543},
};

inline constexpr Field kFields_0475[] = {
    {"DecalName", 0, 8, 0x0010, 0},
    {"Decision", 8, 8, 0x001C, 1043},
};

inline constexpr Field kFields_0476[] = {
    {"Decals", 0, 12, 0x0024, 544},
    {"RegionWeight", 12, 4, 0x0008, 9840},
    {"JointName", 16, 8, 0x0010, 0},
    {"AnimName", 24, 8, 0x0010, 0},
    {"HitFlags", 32, 8, 0x0204, 3256},
    {"MaxAngle", 40, 4, 0x0008, 9841},
    {"MaxDistance", 44, 4, 0x0008, 9842},
    {"RegionId", 48, 1, 0x0104, 3257},
    {"Bidirectional", 49, 1, 0x0014, 2349},
};

inline constexpr Field kFields_0477[] = {
    {"SlashWounds", 0, 12, 0x0024, 545},
};

inline constexpr Field kFields_0478[] = {
    {"DecalA", 0, 8, 0x0010, 0},
    {"DecalB", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0479[] = {
    {"BinaryDecals", 0, 12, 0x0024, 546},
};

inline constexpr Field kFields_047A[] = {
    {"Allowed", 0, 1, 0x0014, 2350},
    {"HeadAngleStart", 4, 4, 0x0008, 9843},
    {"HeadAngleEnd", 8, 4, 0x0008, 9844},
    {"SpineAngleStart", 12, 4, 0x0008, 9845},
    {"SpineAngleEnd", 16, 4, 0x0008, 9846},
};

inline constexpr Field kFields_047B[] = {
    {"ProceduralSpine", 0, 0, 0x002C, 1146},
    {"ImpactSpeedThreshold", 20, 4, 0x0008, 9851},
    {"Friction", 24, 4, 0x0008, 9852},
    {"Bounciness", 28, 4, 0x0008, 9853},
    {"MinBounceSpeed", 32, 4, 0x0008, 9854},
    {"MaxBounceSpeed", 36, 4, 0x0008, 9855},
    {"InitialPushToGroundSpeed", 40, 4, 0x0008, 9856},
    {"BehaviorDurationMin", 44, 4, 0x0008, 9857},
    {"BehaviorDurationMax", 48, 4, 0x0008, 9858},
    {"BounceSpeedForMinDuration", 52, 4, 0x0008, 9859},
    {"BounceSpeedForMaxDuration", 56, 4, 0x0008, 9860},
    {"BounceSpeedMultiplierWhenTumbling", 60, 4, 0x0008, 9861},
};

inline constexpr Field kFields_047C[] = {
    {"MinSpeedOnSlideMaterial", 0, 4, 0x0008, 9862},
    {"MinSpeedOnNormalGround", 4, 4, 0x0008, 9863},
    {"LinearAcceleration", 8, 4, 0x0008, 9864},
    {"LinearAccelerationWhenStuck", 12, 4, 0x0008, 9865},
    {"PushToNavMesh", 16, 1, 0x0014, 2352},
    {"MaxDistanceToNavMesh", 20, 4, 0x0008, 9866},
    {"MaxFacingAdjustmentAngularSpeed", 24, 4, 0x0008, 9867},
    {"RollingAngularMuliplierOnSlideMaterial", 28, 4, 0x0008, 9868},
    {"RollingAngularMuliplierOnNormalGround", 32, 4, 0x0008, 9869},
};

inline constexpr Field kFields_047D[] = {
    {"PushAngle", 0, 4, 0x0008, 9870},
    {"WallCheckDistance", 4, 4, 0x0008, 9871},
    {"Friction", 8, 4, 0x0008, 9872},
    {"Bounciness", 12, 4, 0x0008, 9873},
    {"MinBounceSpeed", 16, 4, 0x0008, 9874},
    {"MaxBounceSpeed", 20, 4, 0x0008, 9875},
    {"MinHeightForNormalRotation", 24, 4, 0x0008, 9876},
    {"RotationMultiplier_HighAndVertical", 28, 4, 0x0008, 9877},
    {"RotationMultiplier_HighAndHorizontal", 32, 4, 0x0008, 9878},
    {"RotationMultiplier_BelowMinHeight", 36, 4, 0x0008, 9879},
    {"PitchRotationMultiplier", 40, 4, 0x0008, 9880},
};

inline constexpr Field kFields_047E[] = {
    {"AnimName", 0, 8, 0x0010, 0},
    {"TweenTime", 8, 4, 0x0008, 9881},
};

inline constexpr Field kFields_047F[] = {
    {"DefaultGravity", 0, 4, 0x0008, 9882},
    {"MaxInitialLinearSpeed", 4, 4, 0x0008, 9883},
    {"MotionParams", 8, 8, 0x001C, 384},
    {"CombatCollision", 16, 0, 0x002C, 396},
    {"SpeedToActivateCombatCollision", 392, 4, 0x0008, 9948},
    {"DefaultBodyProperties", 396, 0, 0x002C, 1549},
    {"GroundPush", 408, 0, 0x002C, 1147},
    {"RollBehavior", 472, 0, 0x002C, 1148},
    {"WallPush", 508, 0, 0x002C, 1149},
    {"ReplaceAnim", 552, 0, 0x002C, 1150},
};

inline constexpr Field kFields_0480[] = {
    {"DefaultParams", 0, 8, 0x001C, 1151},
    {"AlternativeParams", 8, 0, 0x0028, 35},
};

inline constexpr Field kFields_0481[] = {
    {"DefaultGroup", 0, 0, 0x002C, 1152},
    {"AlternativeGroup", 24, 0, 0x0028, 37},
    {"AllowRagdoll", 36, 1, 0x0014, 2361},
    {"IgnoreInternalCollision", 37, 1, 0x0014, 2362},
};

inline constexpr Field kFields_0482[] = {
    {"Part", 0, 12, 0x0024, 553},
    {"KeyPart", 12, 2, 0x0104, 3281},
    {"Weight", 16, 12, 0x0024, 554},
};

inline constexpr Field kFields_0483[] = {
    {"Rows", 0, 12, 0x0024, 555},
    {"Default", 16, 0, 0x002C, 1154},
};

inline constexpr Field kFields_0484[] = {
    {"Difficulty", 0, 8, 0x0010, 0},
    {"Value", 8, 4, 0x0008, 9989},
};

inline constexpr Field kFields_0485[] = {
    {"Attribute", 0, 8, 0x0010, 0},
    {"StatsValue", 8, 12, 0x0024, 558},
};

inline constexpr Field kFields_0486[] = {
    {"DesiredStates", 0, 12, 0x0024, 559},
};

inline constexpr Field kFields_0487[] = {
    {"WalkSpeed", 0, 4, 0x0008, 9990},
    {"JogSpeed", 4, 4, 0x0008, 9991},
    {"RunSpeed", 8, 4, 0x0008, 9992},
    {"SprintSpeed", 12, 4, 0x0008, 9993},
};

inline constexpr Field kFields_0488[] = {
    {"WeaponSwitchList", 0, 12, 0x0024, 560},
};

inline constexpr Field kFields_0489[] = {
    {"WeaponType", 0, 4, 0x0000, 1183},
    {"ActiveJoint", 8, 8, 0x0018, 0},
    {"StowJoint", 16, 8, 0x0018, 0},
};

inline constexpr Field kFields_048A[] = {
    {"JointName", 0, 8, 0x0010, 0},
    {"BodyPart", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_048B[] = {
    {"NumLinks", 0, 1, 0x0000, 1184},
    {"Length", 4, 4, 0x0008, 9994},
    {"Damping", 8, 4, 0x0008, 9995},
    {"Radius", 12, 4, 0x0008, 9996},
    {"FastForwardSimulation", 16, 4, 0x0008, 9997},
    {"AttachmentObjectName", 24, 8, 0x0010, 0},
    {"StartJointName", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_048C[] = {
    {"RopeConfiguration", 0, 12, 0x0024, 561},
    {"AllowAttachedRope", 12, 1, 0x0014, 2363},
};

inline constexpr Field kFields_048D[] = {
    {"DefaultTargetingParameters", 0, 0, 0x002C, 239},
    {"DefaultInteractsTargetingParameters", 128, 0, 0x002C, 239},
    {"DefaultBreakablesTargetingParameters", 256, 0, 0x002C, 239},
    {"DefaultGrapplePointsTargetingParameters", 384, 0, 0x002C, 239},
    {"DefaultTetherCranksTargetingParameters", 512, 0, 0x002C, 239},
    {"LookAtConvoSettings", 640, 0, 0x002C, 1136},
    {"DeathParameters", 736, 0, 0x002C, 511},
    {"ProximityWeight", 776, 0, 0x002C, 229},
    {"ProximityWeightLimit", 816, 0, 0x002C, 229},
    {"Ragdoll", 856, 0, 0x002C, 1153},
    {"SpeedSettings", 896, 0, 0x002C, 1159},
    {"SlashWoundSet", 912, 0, 0x002C, 1143},
    {"BinaryDecalSet", 928, 0, 0x002C, 1145},
    {"AttachmentList", 944, 12, 0x0024, 577},
    {"Flags", 956, 4, 0x0204, 3295},
    {"DefaultAttachments", 960, 12, 0x0024, 578},
    {"MusicIntensity", 972, 4, 0x0000, 1189},
    {"Stats", 976, 12, 0x0024, 579},
    {"Scale", 988, 4, 0x0008, 10152},
    {"EnterHotFx", 992, 12, 0x0024, 580},
    {"PathingTolerance", 1004, 4, 0x0008, 10153},
    {"InheritPlayerTintsTexSwapsGoNames", 1008, 12, 0x0024, 581},
    {"DistanceFromGroundToAirState", 1020, 4, 0x0008, 10154},
    {"WeaponJoints", 1024, 12, 0x0024, 582},
    {"DistanceFromAirToGroundState", 1036, 4, 0x0008, 10155},
    {"WeaponEmbedPoints", 1040, 12, 0x0024, 583},
    {"StickToWallAngle", 1052, 4, 0x0008, 10156},
    {"DefaultStatAttributes", 1056, 12, 0x0024, 584},
    {"AccelBlendDamping", 1068, 4, 0x0008, 10157},
    {"VaryingRadius", 1072, 12, 0x0024, 585},
    {"SpeedBlendDamping", 1084, 4, 0x0008, 10158},
    {"JointBodyPartAssociationArray", 1088, 12, 0x0024, 586},
    {"SlopeBlendDamping", 1100, 4, 0x0008, 10159},
    {"SuckToTargetDefaultScaleAngleList", 1104, 12, 0x0024, 587},
    {"RollBlendDamping", 1116, 4, 0x0008, 10161},
    {"SuckToTargetDefaultScaleDistanceList", 1120, 12, 0x0024, 588},
    {"StrafeDirBlendDamping", 1132, 4, 0x0008, 10163},
    {"ImpulseCameraDefaultAngleList", 1136, 12, 0x0024, 589},
    {"AngleToTargetBlendDamping", 1148, 4, 0x0008, 10165},
    {"ImpulseCameraDefaultExtraAwayValueList", 1152, 12, 0x0024, 590},
    {"AngleToDesiredFacingDamping", 1164, 4, 0x0008, 10167},
    {"InRangePromptName", 1168, 8, 0x0018, 746},
    {"DefaultWeapon", 1176, 8, 0x0010, 0},
    {"DefaultStance", 1184, 8, 0x001C, 329},
    {"AttachmentBreakMove", 1192, 8, 0x001C, 1286},
    {"Handle", 1200, 8, 0x001C, 255},
    {"PhysicalAttackFlags", 1208, 8, 0x0204, 3296},
    {"MagicAttackFlags", 1216, 8, 0x0204, 3297},
    {"LockOnJoint", 1224, 8, 0x0010, 0},
    {"WeaponSwitchMoveLookup", 1232, 8, 0x001C, 1160},
    {"WeaponSwitchModeDesiredState", 1240, 8, 0x001C, 1158},
    {"Tether", 1248, 8, 0x001C, 347},
    {"VehicleControls", 1256, 8, 0x001C, 156},
    {"Vehicle", 1264, 8, 0x001C, 1123},
    {"BoatVehicle", 1272, 8, 0x001C, 1124},
    {"WolfSledParameters", 1280, 8, 0x001C, 1132},
    {"FlumeRideParameters", 1288, 8, 0x001C, 1133},
    {"YakParameters", 1296, 8, 0x001C, 1134},
    {"ParryReaction", 1304, 8, 0x001C, 1286},
    {"ParryMask", 1312, 8, 0x0204, 3298},
    {"BlockMask", 1320, 8, 0x0204, 3299},
    {"RaySourceJoint", 1328, 8, 0x0018, 747},
    {"RagdollHitTable", 1336, 8, 0x001C, 1155},
    {"AttachedRopes", 1344, 8, 0x001C, 1164},
    {"AngleToReticleBlendDamping", 1352, 4, 0x0008, 10168},
    {"AIStrafeDamping", 1356, 4, 0x0008, 10169},
    {"LeftStickDamping", 1360, 4, 0x0008, 10170},
    {"RightStickDamping", 1364, 4, 0x0008, 10171},
    {"PadDirDamping", 1368, 4, 0x0008, 10172},
    {"LeftStickVariableDampingInner", 1372, 4, 0x0008, 10173},
    {"LeftStickVariableDampingMiddle", 1376, 4, 0x0008, 10174},
    {"LeftStickVariableDampingOuter", 1380, 4, 0x0008, 10175},
    {"RightStickVariableDampingInner", 1384, 4, 0x0008, 10176},
    {"RightStickVariableDampingMiddle", 1388, 4, 0x0008, 10177},
    {"RightStickVariableDampingOuter", 1392, 4, 0x0008, 10178},
    {"LeftStickVariableDampingInnerTol", 1396, 4, 0x0008, 10179},
    {"LeftStickVariableDampingMiddleTol", 1400, 4, 0x0008, 10180},
    {"RightStickVariableDampingInnerTol", 1404, 4, 0x0008, 10181},
    {"RightStickVariableDampingMiddleTol", 1408, 4, 0x0008, 10182},
    {"PelvisToZeroJointDamping", 1412, 4, 0x0008, 10183},
    {"LargeCreatureCollisionExtendCapsuleRadius", 1416, 4, 0x0008, 10184},
    {"LandTimer", 1420, 4, 0x0008, 10185},
    {"FlyingMaxSpeed", 1424, 4, 0x0008, 10186},
    {"FlyingAcc", 1428, 4, 0x0008, 10187},
    {"FlyingDec", 1432, 4, 0x0008, 10188},
    {"FlyingTurnDamping", 1436, 4, 0x0008, 10189},
    {"MaxLockOnDistance", 1440, 4, 0x0008, 10190},
    {"LockOnTime", 1444, 4, 0x0008, 10191},
    {"LockOnSpeedBias", 1448, 4, 0x0008, 10192},
    {"LockOnConstraintOverrider", 1452, 4, 0x0008, 10193},
    {"ScreenPenetrationAdjustment", 1456, 4, 0x0008, 10194},
    {"MaxHeadLeftTurnAngle", 1460, 4, 0x0008, 10195},
    {"MaxHeadRightTurnAngle", 1464, 4, 0x0008, 10196},
    {"MaxHeadTiltFwdAngle", 1468, 4, 0x0008, 10197},
    {"MaxHeadTiltBackAngle", 1472, 4, 0x0008, 10198},
    {"NeckAngularAccel", 1476, 4, 0x0008, 10199},
    {"NeckMaxAngularVel", 1480, 4, 0x0008, 10200},
    {"HeadLookAtInfluenceFactor", 1484, 4, 0x0008, 10201},
    {"ChestLookAtInfluenceFactor", 1488, 4, 0x0008, 10202},
    {"HeadLookAtRollDamping", 1492, 4, 0x0008, 10203},
    {"HeadLookAtPitchDamping", 1496, 4, 0x0008, 10204},
    {"ChestLookAtPitchDamping", 1500, 4, 0x0008, 10205},
    {"EaseInSpeed", 1504, 4, 0x0008, 10206},
    {"EaseOutSpeed", 1508, 4, 0x0008, 10207},
    {"StrafeLeftForward", 1512, 4, 0x0008, 10208},
    {"StrafeLeftBackward", 1516, 4, 0x0008, 10209},
    {"StrafeRightForward", 1520, 4, 0x0008, 10210},
    {"StrafeRightBackward", 1524, 4, 0x0008, 10211},
    {"ForwardStrafeBiasLeft", 1528, 4, 0x0008, 10212},
    {"ForwardStrafeBiasRight", 1532, 4, 0x0008, 10213},
    {"BackwardStrafeBiasLeft", 1536, 4, 0x0008, 10214},
    {"BackwardStrafeBiasRight", 1540, 4, 0x0008, 10215},
    {"FlushStrafeBiasRight", 1544, 4, 0x0008, 10216},
    {"FlushStrafeBiasLeft", 1548, 4, 0x0008, 10217},
    {"BiasTimer", 1552, 4, 0x0008, 10218},
    {"TurnOnSpotFramesToLookAhead", 1556, 4, 0x0008, 10219},
    {"MaxChangePerSecondNormalizedPadDir", 1560, 4, 0x0008, 10220},
    {"PoseMatchWalkStopDistance", 1564, 4, 0x0008, 10221},
    {"PoseMatchRunStopDistance", 1568, 4, 0x0008, 10222},
    {"ShootingWeightRampUp", 1572, 4, 0x0008, 10223},
    {"ShootingWeightCoolDown", 1576, 4, 0x0008, 10224},
    {"RadiusModForDistanceEstimation", 1580, 4, 0x0008, 10225},
    {"FightWeightLimit", 1584, 2, 0x0000, 1190},
    {"ProjectileWeightLimit", 1586, 2, 0x0000, 1191},
    {"CreatureCategory", 1588, 1, 0x0104, 3300},
    {"InRangeButton", 1589, 1, 0x0104, 3301},
    {"InRangeFirstTimeMsg", 1590, 1, 0x0000, 1192},
    {"AudioVariations", 1591, 1, 0x0000, 1193},
    {"WeaponTrailOversamples", 1592, 1, 0x0000, 1194},
    {"UprightHeadTrack", 1593, 1, 0x0014, 2380},
    {"StrafePrototype", 1594, 1, 0x0014, 2381},
    {"StrafeTurnOnSpot", 1595, 1, 0x0014, 2382},
    {"UseStrafeBiasRestrictZones", 1596, 1, 0x0014, 2383},
    {"UseLocomotionAcceleration", 1597, 1, 0x0014, 2384},
    {"DisplayBasicHealthBar", 1598, 1, 0x0014, 2385},
    {"UseDynamicTargetForSuckToTarget", 1599, 1, 0x0014, 2386},
    {"ShouldUpdateVehicleInventory", 1600, 1, 0x0014, 2387},
    {"DisableArrowAcquireNonReticleTarget", 1601, 1, 0x0014, 2388},
};

inline constexpr Field kFields_048E[] = {
    {"Threshold", 0, 4, 0x0008, 10226},
    {"Anim", 8, 8, 0x0010, 0},
    {"PlayFXList", 16, 8, 0x001C, 273},
};

inline constexpr Field kFields_048F[] = {
    {"Scale", 0, 4, 0x0008, 10227},
    {"XOffset", 4, 4, 0x0008, 10228},
    {"YOffset", 8, 4, 0x0008, 10229},
    {"ZOffset", 12, 4, 0x0008, 10230},
    {"SoundDelay", 16, 4, 0x0008, 10231},
    {"JointName", 24, 8, 0x0010, 0},
    {"GOName", 32, 8, 0x0010, 748},
};

inline constexpr Field kFields_0490[] = {
    {"LayerList", 0, 12, 0x0024, 591},
};

inline constexpr Field kFields_0491[] = {
    {"PartFlags", 0, 8, 0x0204, 3302},
    {"ThrowableResponse", 8, 8, 0x0010, 0},
    {"HitTrackJoint", 16, 8, 0x0010, 0},
    {"HitDirection", 24, 0, 0x002C, 6},
    {"ID", 30, 1, 0x0000, 1195},
};

inline constexpr Field kFields_0492[] = {
    {"ObstructedGraceTime", 0, 4, 0x0008, 10235},
    {"ObstructedTimeoutForDroppingAggressivePriority", 4, 4, 0x0008, 10236},
    {"OffscreenGraceTime", 8, 4, 0x0008, 10237},
};

inline constexpr Field kFields_0493[] = {
    {"FightStanceValues", 0, 0, 0x002C, 179},
    {"CircleList", 16, 12, 0x0024, 592},
    {"StunThreshold", 28, 4, 0x0008, 10238},
    {"CollisionSphereList", 32, 12, 0x0024, 593},
    {"StunDrainRate", 44, 4, 0x0008, 10239},
    {"HealthAnimList", 48, 12, 0x0024, 594},
    {"FightSystemConfig", 60, 0, 0x002C, 1170},
    {"ID", 72, 8, 0x0010, 0},
    {"Context", 80, 8, 0x0010, 0},
    {"BlockHitFlags", 88, 8, 0x0204, 3303},
    {"PartMask", 96, 8, 0x0204, 3304},
    {"DefaultStance", 104, 8, 0x001C, 339},
    {"AnimLayers", 112, 8, 0x001C, 1168},
    {"DebugInfoJoint", 120, 8, 0x0010, 0},
    {"StunDrainDelay", 128, 4, 0x0008, 10243},
    {"DamageMultiplier", 132, 4, 0x0008, 10244},
    {"Tags", 136, 4, 0x0204, 3305},
    {"AvoidanceCircleRadius", 140, 4, 0x0008, 10245},
    {"CollisionDefaultID", 144, 4, 0x0000, 1197},
    {"ImpulseAwayScale", 148, 4, 0x0008, 10246},
    {"ImpulseUpScale", 152, 4, 0x0008, 10247},
    {"ImpulseRightScale", 156, 4, 0x0008, 10248},
    {"AutoAimDampingDefault", 160, 4, 0x0008, 10249},
    {"AutoAimDampingFast", 164, 4, 0x0008, 10250},
    {"AutoAimDampingMedium", 168, 4, 0x0008, 10251},
    {"AutoAimDampingSlow", 172, 4, 0x0008, 10252},
    {"AutoAimDampingGlacial", 176, 4, 0x0008, 10253},
    {"SynchDampingDefault", 180, 4, 0x0008, 10254},
    {"SynchDampingFast", 184, 4, 0x0008, 10255},
    {"SynchDampingMedium", 188, 4, 0x0008, 10256},
    {"SynchDampingSlow", 192, 4, 0x0008, 10257},
    {"SynchDampingGlacial", 196, 4, 0x0008, 10258},
    {"AttachmentDampingDefault", 200, 4, 0x0008, 10259},
    {"AttachmentDampingFast", 204, 4, 0x0008, 10260},
    {"AttachmentDampingMedium", 208, 4, 0x0008, 10261},
    {"AttachmentDampingSlow", 212, 4, 0x0008, 10262},
    {"AttachmentDampingGlacial", 216, 4, 0x0008, 10263},
    {"Team", 220, 1, 0x0104, 3306},
    {"CameraBossFocus", 221, 1, 0x0014, 2390},
};

inline constexpr Field kFields_0494[] = {
    {"Flags", 0, 2, 0x0204, 3307},
    {"Duration", 4, 4, 0x0008, 10264},
    {"Radius", 8, 4, 0x0008, 10265},
    {"HitResistance", 12, 4, 0x0008, 10266},
    {"ContextFilter", 16, 4, 0x0000, 1198},
    {"TargetFilter", 20, 4, 0x0000, 1199},
    {"FXReleaseShard", 24, 8, 0x0010, 0},
    {"FXAbsorbShard", 32, 8, 0x0010, 0},
    {"FXShieldHit", 40, 8, 0x0010, 0},
    {"SoundFXReleaseShard", 48, 8, 0x0010, 0},
    {"SoundFXAbsorbShard", 56, 8, 0x0010, 0},
    {"SoundFXShieldHit", 64, 8, 0x0010, 0},
    {"NbShards", 72, 1, 0x0000, 1200},
    {"NbMinShards", 73, 1, 0x0000, 1201},
    {"NbFreedShardsOnHit", 74, 1, 0x0000, 1202},
    {"ReleasedShardRadius", 76, 4, 0x0008, 10267},
    {"ShardTargetingRadius", 80, 4, 0x0008, 10268},
    {"ShardRoamingRadiusMin", 84, 4, 0x0008, 10269},
    {"ShardRoamingRadiusMax", 88, 4, 0x0008, 10270},
    {"ShardInfo", 96, 0, 0x002C, 1556},
};

inline constexpr Field kFields_0495[] = {
    {"ID", 0, 8, 0x0010, 0},
    {"Branch", 8, 8, 0x001C, 1286},
};

inline constexpr Field kFields_0496[] = {
    {"ZeroJoint", 0, 8, 0x0010, 749},
    {"SynchJoint", 8, 8, 0x0010, 750},
    {"LinkJoint", 16, 8, 0x0010, 751},
};

inline constexpr Field kFields_0497[] = {
    {"RequestTypeNames", 0, 12, 0x0024, 595},
};

inline constexpr Field kFields_0498[] = {
    {"MoveEvent", 0, 8, 0x0010, 0},
    {"MinDistanceAway", 8, 4, 0x0008, 10284},
    {"MaxDistanceAway", 12, 4, 0x0008, 10285},
    {"Cooldown", 16, 4, 0x0008, 10286},
    {"OneOffWeight", 20, 1, 0x0000, 1205},
    {"MustBeOnScreen", 21, 1, 0x0014, 2391},
    {"PlayerSide", 22, 1, 0x0104, 3309},
};

inline constexpr Field kFields_0499[] = {
    {"MoveEvent", 0, 8, 0x0018, 0},
    {"OneOffWeight", 8, 1, 0x0000, 1206},
};

inline constexpr Field kFields_049A[] = {
    {"LoopBanter", 0, 12, 0x0024, 596},
    {"CooldownTimer", 12, 4, 0x0008, 10287},
    {"OneOffBanter", 16, 12, 0x0024, 597},
    {"OneOffTimerMax", 28, 4, 0x0008, 10288},
    {"LoopAnimEvents", 32, 12, 0x0024, 598},
    {"OneOffTimerMin", 44, 4, 0x0008, 10289},
    {"OneOffs", 48, 12, 0x0024, 599},
    {"ContextType", 60, 1, 0x0104, 3310},
    {"OneOffsPlayOffScreen", 61, 1, 0x0014, 2392},
    {"ConditionalOneOffs", 64, 12, 0x0024, 600},
    {"EnterBanter", 80, 8, 0x0018, 0},
    {"ExitBanter", 88, 8, 0x0018, 0},
    {"EnterAnimEvent", 96, 8, 0x0018, 0},
    {"ExitAnimEvent", 104, 8, 0x0018, 0},
    {"BanterModeEnterEvent", 112, 8, 0x0018, 0},
    {"BanterModeExitEvent", 120, 8, 0x0018, 0},
    {"BanterModeLoopEvent", 128, 8, 0x0018, 0},
    {"TransitionToComplimentEvent", 136, 8, 0x0018, 0},
    {"ComplimentaryConfigName", 144, 8, 0x0018, 0},
};

inline constexpr Field kFields_049B[] = {
    {"Elements", 0, 12, 0x0024, 601},
};

inline constexpr Field kFields_049C[] = {
    {"JointName", 0, 8, 0x0010, 0},
    {"MarkerID", 8, 8, 0x0010, 0},
    {"PointTest", 16, 1, 0x0014, 2393},
};

inline constexpr Field kFields_049D[] = {
    {"GOName", 0, 8, 0x0018, 0},
    {"Soldier", 8, 8, 0x001C, 1165},
    {"AI", 16, 8, 0x001C, 1116},
    {"Combat", 24, 8, 0x001C, 1171},
    {"Shield", 32, 8, 0x001C, 1172},
    {"FX", 40, 0, 0x002C, 349},
    {"Joints", 416, 0, 0x002C, 1174},
    {"Markers", 440, 12, 0x0024, 606},
    {"OcclusionVolume", 456, 8, 0x001C, 1179},
    {"CameraTargets", 464, 12, 0x0024, 607},
    {"IncludeTraverselinkFilter", 476, 2, 0x0204, 3314},
    {"ExcludeTraverselinkFilter", 478, 2, 0x0204, 3315},
    {"MoveSystemList", 480, 12, 0x0024, 608},
    {"MoveList", 496, 12, 0x0024, 609},
    {"BranchList", 512, 12, 0x0024, 610},
    {"CustomSynchList", 528, 12, 0x0024, 611},
    {"DefaultNavStack", 544, 12, 0x0024, 612},
    {"pointer_9", 560, 8, 0x001C, 1282},
    {"DefaultCombatStack", 568, 12, 0x0024, 613},
    {"pointer_10", 584, 8, 0x001C, 1282},
    {"DefaultStack", 592, 8, 0x001C, 1282},
    {"DefaultMove", 600, 8, 0x001C, 1286},
    {"MotionMap", 608, 0, 0x0028, 40},
    {"NavBankList", 624, 12, 0x0024, 614},
    {"Meters", 640, 12, 0x0024, 615},
    {"HealthMeter", 656, 0, 0x002C, 292},
    {"BTR_RootName", 816, 8, 0x0010, 0},
    {"BTR_FullPath", 824, 8, 0x0010, 0},
    {"BTR_CreatureIdentifier", 832, 8, 0x0010, 0},
    {"EquipmentCharacterSlotSet", 840, 8, 0x0010, 0},
    {"DefaultLootTable", 848, 8, 0x0010, 0},
    {"DefaultLootDistributor", 856, 8, 0x0010, 0},
    {"DynamicFlags", 864, 12, 0x0024, 619},
    {"UsesFlightVolume", 876, 1, 0x0014, 2394},
    {"InteractPromptIconName", 880, 8, 0x0010, 0},
    {"InteractPromptJointName", 888, 8, 0x0010, 0},
    {"InteractJointTargetOverride", 896, 8, 0x0010, 0},
    {"PlayerMemory", 904, 8, 0x001C, 296},
};

inline constexpr Field kFields_049E[] = {
    {"MoveList", 0, 12, 0x0024, 620},
    {"Flags", 12, 1, 0x0204, 3326},
    {"CharacterConfigList", 16, 12, 0x0024, 621},
    {"DeathSoundName", 32, 8, 0x0010, 0},
    {"DeathEffectName", 40, 8, 0x0010, 0},
    {"DeathEffectJoint", 48, 8, 0x0010, 0},
};

inline constexpr Field kFields_049F[] = {
    {"PickupId", 0, 2, 0x0000, 1210},
    {"Flags", 2, 1, 0x0204, 3327},
    {"StageId", 3, 1, 0x0000, 1211},
};

inline constexpr Field kFields_04A0[] = {
    {"Slot", 0, 1, 0x0000, 1212},
    {"HeapSize", 4, 4, 0x0000, 1213},
};

inline constexpr Field kFields_04A1[] = {
    {"Pickup", 0, 2, 0x0000, 1214},
    {"MinStage", 2, 1, 0x0000, 1215},
    {"MaxStage", 3, 1, 0x0000, 1216},
    {"Tags", 4, 4, 0x0204, 3328},
    {"State", 8, 1, 0x0104, 3329},
};

inline constexpr Field kFields_04A2[] = {
    {"Tags", 0, 4, 0x0204, 3330},
    {"Slot", 4, 1, 0x0000, 1217},
    {"MinStage", 5, 1, 0x0000, 1218},
    {"MaxStage", 6, 1, 0x0000, 1219},
    {"State", 7, 1, 0x0104, 3331},
};

inline constexpr Field kFields_04A3[] = {
    {"Pickup", 40, 2, 0x0000, 1221},
    {"InitialBranchName", 48, 8, 0x0010, 0},
};

inline constexpr Field kFields_04A4[] = {
    {"PickupFlags", 24, 1, 0x0204, 3341},
    {"Pickup", 26, 2, 0x0000, 1223},
    {"Stage", 28, 1, 0x0000, 1224},
    {"Index", 30, 2, 0x0000, 1225},
};

inline constexpr Field kFields_04A5[] = {
    {"AdjustTime", 0, 0, 0x002C, 229},
    {"ScaleTime", 40, 0, 0x002C, 229},
    {"AdjustByPercentMax", 80, 0, 0x002C, 229},
    {"RefreshSlot", 120, 1, 0x0000, 1226},
    {"CapToCurrentValue", 121, 1, 0x0014, 2395},
};

inline constexpr Field kFields_04A6[] = {
    {"RefreshCooldown", 24, 8, 0x001C, 1189},
};

inline constexpr Field kFields_04A7[] = {
    {"Pickup", 24, 2, 0x0000, 1229},
};

inline constexpr Field kFields_04A8[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Slot", 8, 1, 0x0000, 1230},
    {"CooldownType", 9, 1, 0x0104, 3359},
};

inline constexpr Field kFields_04A9[] = {
    {"Heap", 0, 1, 0x0104, 3360},
    {"Name", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_04AA[] = {
    {"PickupId", 0, 2, 0x0000, 1231},
    {"StageId", 2, 1, 0x0000, 1232},
    {"Money", 4, 4, 0x0000, 1233},
};

inline constexpr Field kFields_04AB[] = {
    {"When", 0, 8, 0x0030, 65535},
    {"RequiredWeaponMode", 8, 8, 0x0010, 0},
    {"On", 16, 4, 0x0008, 10356},
    {"Off", 20, 4, 0x0008, 10357},
    {"RequiredPickup", 24, 2, 0x0000, 1234},
    {"Type", 26, 1, 0x0104, 3361},
    {"Flags", 27, 1, 0x0204, 3362},
    {"Visibility", 28, 1, 0x0204, 3363},
    {"AvailableInDifficulty", 29, 1, 0x0204, 3364},
    {"OnWhenAcquired", 30, 1, 0x0014, 2396},
    {"HardcodedExpression", 31, 1, 0x0104, 3365},
};

inline constexpr Field kFields_04AC[] = {
    {"Duration", 32, 0, 0x002C, 229},
    {"CanRefresh", 72, 1, 0x0014, 2398},
    {"UseCreatureTime", 73, 1, 0x0014, 2399},
};

inline constexpr Field kFields_04AD[] = {
    {"Duration", 32, 4, 0x0008, 10368},
};

inline constexpr Field kFields_04AE[] = {
    {"Modifiers", 32, 12, 0x0024, 626},
};

inline constexpr Field kFields_04AF[] = {
    {"Attribute", 0, 8, 0x0010, 0},
    {"Value", 8, 4, 0x0008, 10371},
};

inline constexpr Field kFields_04B0[] = {
    {"Attributes", 32, 12, 0x0024, 627},
};

inline constexpr Field kFields_04B1[] = {
    {"Add", 32, 4, 0x0204, 3394},
    {"Remove", 36, 4, 0x0204, 3395},
};

inline constexpr Field kFields_04B2[] = {
    {"Add", 32, 4, 0x0204, 3401},
};

inline constexpr Field kFields_04B3[] = {
    {"Pickups", 32, 12, 0x0024, 628},
    {"AutoRemove", 44, 1, 0x0014, 2406},
    {"OverwritePickupStageIfAcquired", 45, 1, 0x0014, 2407},
};

inline constexpr Field kFields_04B4[] = {
    {"OverrideMaterialFXFrom", 32, 8, 0x0010, 0},
    {"OverrideMaterialFXTo", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_04B5[] = {
    {"MFXSwitches", 32, 4, 0x0024, 629},
};

inline constexpr Field kFields_04B6[] = {
    {"Pickups", 32, 12, 0x0024, 630},
    {"GiveRadius", 44, 4, 0x0008, 10386},
    {"IgnoreList", 48, 12, 0x0024, 631},
    {"RevokeRadius", 60, 4, 0x0008, 10387},
    {"Targets", 64, 1, 0x0204, 3422},
};

inline constexpr Field kFields_04B7[] = {
    {"Name", 32, 12, 0x0024, 632},
    {"Priority", 44, 4, 0x0000, 1247},
    {"val_11", 48, 8, 0x0018, 0},
};

inline constexpr Field kFields_04B8[] = {
    {"Template", 32, 8, 0x0010, 0},
    {"Config", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_04B9[] = {
    {"WeaponType", 32, 4, 0x0000, 1250},
    {"RestoreState", 36, 1, 0x0104, 3438},
};

inline constexpr Field kFields_04BA[] = {
    {"Scale", 32, 4, 0x0008, 10396},
    {"ApplyTween", 36, 0, 0x002C, 12},
    {"RemoveTween", 60, 0, 0x002C, 12},
};

inline constexpr Field kFields_04BB[] = {
    {"Alpha", 32, 4, 0x0008, 10411},
    {"ApplyTween", 36, 0, 0x002C, 12},
    {"RemoveTween", 60, 0, 0x002C, 12},
    {"Heap", 84, 4, 0x0000, 1253},
    {"ApplyEffect", 88, 8, 0x001C, 294},
    {"RemoveEffect", 96, 8, 0x001C, 294},
};

inline constexpr Field kFields_04BC[] = {
    {"ExposedSideName", 32, 8, 0x0010, 0},
    {"Shield", 40, 4, 0x0008, 10426},
    {"ShieldDefaultHitModifierList", 48, 12, 0x0024, 633},
    {"KeepOn", 60, 1, 0x0014, 2417},
};

inline constexpr Field kFields_04BD[] = {
    {"HitModifier", 32, 8, 0x001C, 343},
};

inline constexpr Field kFields_04BE[] = {
    {"ModifierFlags", 32, 1, 0x0204, 3464},
};

inline constexpr Field kFields_04BF[] = {
    {"SlotIds", 0, 12, 0x0024, 634},
};

inline constexpr Field kFields_04C0[] = {
    {"Name", 32, 8, 0x0010, 0},
    {"SwapMaterialName", 40, 8, 0x0010, 0},
    {"AttachmentName", 48, 8, 0x0010, 0},
    {"AttachName", 56, 8, 0x0010, 0},
    {"WeaponType", 64, 4, 0x0000, 1259},
};

inline constexpr Field kFields_04C2[] = {
    {"Voice", 32, 1, 0x0000, 1262},
};

inline constexpr Field kFields_04C3[] = {
    {"Attributes", 32, 8, 0x0204, 3485},
};

inline constexpr Field kFields_04C4[] = {
    {"PrimaryJoint", 32, 8, 0x0010, 0},
    {"AttachName", 40, 8, 0x0010, 0},
    {"YPositionJoint", 48, 8, 0x0010, 0},
    {"Offset", 56, 0, 0x002C, 6},
    {"Rotation", 62, 0, 0x002C, 6},
    {"Radius", 68, 4, 0x0008, 10447},
    {"Length", 72, 4, 0x0008, 10448},
};

inline constexpr Field kFields_04C5[] = {
    {"Scale", 32, 4, 0x0008, 10451},
    {"ApplyTween", 36, 0, 0x002C, 12},
    {"RemoveTween", 60, 0, 0x002C, 12},
};

inline constexpr Field kFields_04C6[] = {
    {"IconName", 32, 8, 0x0010, 0},
    {"JointName", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_04C7[] = {
    {"Meter", 32, 8, 0x001C, 292},
};

inline constexpr Field kFields_04C8[] = {
    {"Timeout", 32, 0, 0x002C, 229},
    {"CanRefresh", 72, 1, 0x0014, 2429},
};

inline constexpr Field kFields_04C9[] = {
    {"RefreshCooldown", 32, 8, 0x001C, 1189},
};

inline constexpr Field kFields_04CA[] = {
    {"WeaponTrailData", 32, 8, 0x001C, 237},
};

inline constexpr Field kFields_04CB[] = {
    {"WeaponTrailType", 32, 8, 0x0010, 0},
    {"TrailName", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_04CC[] = {
    {"SnapData", 32, 8, 0x001C, 766},
};

inline constexpr Field kFields_04CD[] = {
    {"BlockName", 32, 8, 0x0010, 0},
    {"BlockData", 40, 8, 0x001C, 520},
};

inline constexpr Field kFields_04CE[] = {
    {"Originator", 32, 1, 0x0104, 3544},
    {"Receiver", 33, 1, 0x0104, 3545},
    {"Rate", 36, 4, 0x0008, 10488},
    {"Emitter", 40, 8, 0x001C, 321},
};

inline constexpr Field kFields_04CF[] = {
    {"OrbType", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_04D0[] = {
    {"Heap", 32, 4, 0x0000, 1277},
    {"Effects", 40, 8, 0x001C, 273},
};

inline constexpr Field kFields_04D2[] = {
    {"Heap", 32, 4, 0x0000, 1281},
    {"Name", 40, 8, 0x0018, 0},
    {"ApplyTween", 48, 0, 0x002C, 12},
    {"RemoveTween", 72, 0, 0x002C, 12},
};

inline constexpr Field kFields_04D3[] = {
    {"Name", 32, 8, 0x0010, 0},
    {"EmitterName", 40, 8, 0x0018, 0},
    {"RingOut", 48, 1, 0x0014, 2441},
    {"Gain", 52, 4, 0x0008, 10511},
    {"Inner", 56, 4, 0x0008, 10512},
    {"Outer", 60, 4, 0x0008, 10513},
};

inline constexpr Field kFields_04D4[] = {
    {"RTPCName", 32, 8, 0x0018, 0},
    {"RTPCValue", 40, 4, 0x0008, 10516},
};

inline constexpr Field kFields_04D5[] = {
    {"GroupName", 32, 8, 0x0018, 0},
    {"State", 40, 8, 0x0018, 0},
    {"EmitterName", 48, 8, 0x0018, 0},
};

inline constexpr Field kFields_04D6[] = {
    {"GroupName", 32, 8, 0x0018, 0},
    {"State", 40, 8, 0x0018, 0},
};

inline constexpr Field kFields_04D7[] = {
    {"MeterName", 32, 8, 0x0010, 0},
    {"ModInitialAmount", 40, 8, 0x001C, 229},
    {"ModTickAmount", 48, 8, 0x001C, 229},
    {"ModOnRemoveAmount", 56, 8, 0x001C, 229},
    {"InitialAmountMeterName", 64, 8, 0x0010, 0},
    {"TickAmountMeterName", 72, 8, 0x0010, 0},
    {"OnRemoveAmountMeterName", 80, 8, 0x0010, 0},
    {"HitFlags", 88, 8, 0x0204, 3591},
    {"AttackerAdjustment", 96, 8, 0x0010, 771},
    {"DefenderAdjustment", 104, 8, 0x0010, 772},
    {"InitialAmount", 112, 4, 0x0008, 10523},
    {"TickAmount", 116, 4, 0x0008, 10524},
    {"OnRemoveAmount", 120, 4, 0x0008, 10525},
    {"TickInterval", 124, 4, 0x0008, 10526},
    {"IsInitialAmountAbsolute", 128, 1, 0x0014, 2446},
    {"IsOnRemoveAmountAbsolute", 129, 1, 0x0014, 2447},
    {"PauseMeter", 130, 1, 0x0104, 3592},
    {"OnExitPauseMeter", 131, 1, 0x0104, 3593},
    {"HealthMeterUseDamagePipeline", 132, 1, 0x0014, 2448},
    {"UseStats", 133, 1, 0x0014, 2449},
    {"DoNotRemovePickupIfFailed", 134, 1, 0x0014, 2450},
    {"OverrideImmune", 135, 1, 0x0014, 2451},
    {"TriggerAttackerHitEvent", 136, 1, 0x0014, 2452},
    {"TriggerCollisionEventForHealth", 137, 1, 0x0014, 2453},
    {"IgnoreInvulnerable", 138, 1, 0x0014, 2454},
    {"DamageSource", 139, 1, 0x0104, 3594},
};

inline constexpr Field kFields_04D8[] = {
    {"MeterRedirectFrom", 0, 8, 0x0010, 0},
    {"MeterRedirectTo", 8, 8, 0x0010, 0},
    {"RedirectType", 16, 1, 0x0104, 3595},
    {"ScaleOriginal", 20, 4, 0x0008, 10527},
    {"ScaleRedirect", 24, 4, 0x0008, 10528},
};

inline constexpr Field kFields_04D9[] = {
    {"RedirectData", 32, 8, 0x001C, 1240},
};

inline constexpr Field kFields_04DA[] = {
    {"Enabled", 32, 1, 0x0014, 2457},
};

inline constexpr Field kFields_04DB[] = {
    {"EmissiveScale", 32, 4, 0x0008, 10535},
    {"EaseInTime", 36, 4, 0x0008, 10536},
    {"EaseOutTime", 40, 4, 0x0008, 10537},
};

inline constexpr Field kFields_04DC[] = {
    {"Activate", 32, 8, 0x0010, 0},
    {"Deactivate", 40, 8, 0x0010, 0},
    {"Death", 48, 8, 0x0010, 0},
};

inline constexpr Field kFields_04DD[] = {
    {"Concussion", 32, 8, 0x001C, 268},
    {"Heap", 40, 4, 0x0000, 1292},
    {"Rate", 44, 4, 0x0008, 10542},
};

inline constexpr Field kFields_04DE[] = {
    {"RakeMode", 32, 1, 0x0104, 3626},
    {"MaxDistance", 36, 4, 0x0008, 10545},
    {"Speed", 40, 4, 0x0008, 10546},
    {"ConcussionMinLength", 44, 4, 0x0008, 10547},
    {"StartAngle", 48, 4, 0x0008, 10548},
    {"RandomOffsetAngle", 52, 4, 0x0008, 10549},
    {"MaxHeightDelta", 56, 4, 0x0008, 10550},
    {"RotationSpeed", 60, 4, 0x0008, 10551},
    {"ConcussionAttachToMovingPlatform", 64, 1, 0x0014, 2462},
    {"Heap", 68, 4, 0x0000, 1294},
    {"Concussion", 72, 8, 0x001C, 268},
};

inline constexpr Field kFields_04DF[] = {
    {"ApplyAnimation", 32, 8, 0x0010, 0},
    {"RemoveAnimation", 40, 8, 0x0010, 0},
    {"TimeScaleBehavior", 48, 1, 0x0104, 3632},
    {"WeaponType", 52, 4, 0x0000, 1296},
    {"AttachName", 56, 8, 0x0010, 0},
    {"ChildObjectName", 64, 8, 0x0010, 0},
    {"IconName", 72, 8, 0x0010, 0},
    {"PlaybackSpeed", 80, 4, 0x0008, 10554},
    {"StartPos", 84, 4, 0x0008, 10555},
};

inline constexpr Field kFields_04E0[] = {
    {"ReticleTargetGroup", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_04E1[] = {
    {"AimAssist", 32, 1, 0x0014, 2466},
};

inline constexpr Field kFields_04E2[] = {
    {"Amount", 32, 4, 0x0008, 10562},
    {"Material", 36, 1, 0x0104, 3648},
    {"LayerMask", 40, 4, 0x0204, 3649},
    {"FadeInTime", 44, 4, 0x0008, 10563},
    {"FadeOutTime", 48, 4, 0x0008, 10564},
    {"ImmediateRemovalOnExit", 52, 1, 0x0014, 2468},
};

inline constexpr Field kFields_04E3[] = {
    {"Name", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_04E4[] = {
    {"Effect", 32, 8, 0x001C, 46},
    {"Duration", 40, 4, 0x0008, 10569},
    {"Weight", 44, 4, 0x0008, 10570},
};

inline constexpr Field kFields_04E5[] = {
    {"HealthBarState", 32, 1, 0x0104, 3665},
    {"Global", 33, 1, 0x0014, 2472},
};

inline constexpr Field kFields_04E6[] = {
    {"Flag", 32, 8, 0x0010, 0},
    {"ForceClear", 40, 1, 0x0014, 2474},
};

inline constexpr Field kFields_04E7[] = {
    {"TimeBubbleFlags", 32, 1, 0x0204, 3676},
    {"Radius", 36, 4, 0x0008, 10577},
    {"TimeScale", 40, 4, 0x0008, 10578},
};

inline constexpr Field kFields_04E8[] = {
    {"Name", 32, 8, 0x0018, 0},
    {"Set", 40, 8, 0x0010, 0},
    {"Suppress", 48, 1, 0x0014, 2477},
};

inline constexpr Field kFields_04E9[] = {
    {"Reacquire", 32, 1, 0x0014, 2479},
    {"OffScreen", 33, 1, 0x0014, 2480},
    {"Invisible", 34, 1, 0x0014, 2481},
};

inline constexpr Field kFields_04EA[] = {
    {"Label", 32, 8, 0x0010, 0},
    {"StringData", 40, 8, 0x0010, 0},
    {"NumberData", 48, 4, 0x0008, 10585},
    {"ApplyFlags", 52, 1, 0x0204, 3692},
    {"Increment", 53, 1, 0x0014, 2483},
};

inline constexpr Field kFields_04EB[] = {
    {"FactName", 32, 8, 0x0010, 0},
    {"Value", 40, 4, 0x0008, 10588},
};

inline constexpr Field kFields_04EC[] = {
    {"FactName", 32, 8, 0x0010, 0},
    {"Value", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_04ED[] = {
    {"FactName", 32, 8, 0x0010, 0},
    {"Value", 40, 1, 0x0014, 2487},
};

inline constexpr Field kFields_04EE[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Hash", 8, 8, 0x0010, 0},
    {"Type", 16, 4, 0x0000, 1311},
};

inline constexpr Field kFields_04EF[] = {
    {"Id", 0, 0, 0x002C, 1262},
    {"Value", 24, 4, 0x0000, 1313},
    {"Type", 28, 1, 0x0104, 3708},
};

inline constexpr Field kFields_04F0[] = {
    {"Id", 0, 0, 0x002C, 1262},
    {"Value", 24, 4, 0x0000, 1315},
    {"Type", 28, 1, 0x0104, 3709},
    {"Operator", 29, 1, 0x0104, 3710},
};

inline constexpr Field kFields_04F1[] = {
    {"Id", 0, 0, 0x002C, 1262},
    {"Value", 24, 4, 0x0000, 1317},
    {"Probability", 28, 4, 0x0008, 10593},
    {"Event", 32, 1, 0x0104, 3711},
    {"Stage", 33, 1, 0x0000, 1318},
};

inline constexpr Field kFields_04F2[] = {
    {"Cooldown", 0, 0, 0x002C, 229},
    {"UnlockConditions", 40, 12, 0x0024, 637},
    {"DisplayStageId", 52, 4, 0x0000, 1319},
    {"ActivateConditions", 56, 12, 0x0024, 638},
    {"DisplayStageDescId", 68, 4, 0x0000, 1320},
    {"Triggers", 72, 12, 0x0024, 639},
    {"DisplayStageFloatValue", 84, 4, 0x0008, 10600},
    {"Features", 88, 12, 0x0024, 640},
    {"RequiredWad", 104, 8, 0x0018, 0},
};

inline constexpr Field kFields_04F3[] = {
    {"Features", 0, 12, 0x0024, 641},
};

inline constexpr Field kFields_04F4[] = {
    {"StageList", 0, 12, 0x0024, 642},
    {"Tags", 12, 4, 0x0204, 3715},
    {"Name", 16, 8, 0x0018, 0},
    {"RequiredWad", 24, 8, 0x0018, 0},
    {"CommonStage", 32, 8, 0x001C, 1267},
    {"DroppedPickup", 40, 8, 0x0018, 0},
    {"IconId", 48, 4, 0x0000, 1321},
    {"DisplayNameId", 52, 4, 0x0000, 1322},
    {"DisplayDescriptionId", 56, 4, 0x0000, 1323},
    {"SecondDisplayDescriptionId", 60, 4, 0x0000, 1324},
    {"Pickup", 64, 2, 0x0000, 1325},
    {"Flags", 66, 1, 0x0204, 3716},
    {"Type", 67, 1, 0x0204, 3717},
    {"Lifetime", 68, 1, 0x0104, 3718},
    {"Slot", 69, 1, 0x0000, 1326},
    {"DLCSlot", 70, 1, 0x0000, 1327},
    {"Button", 71, 1, 0x0104, 3719},
};

inline constexpr Field kFields_04F5[] = {
    {"ReticleAssistEnableThreshold", 72, 4, 0x0008, 10601},
    {"ReticleAssistRayLength", 76, 4, 0x0008, 10602},
    {"ReticleAssistSlowFactor", 80, 4, 0x0008, 10603},
    {"ReticleAssistRampMagnitude", 84, 4, 0x0008, 10604},
};

inline constexpr Field kFields_04F6[] = {
    {"Modifiers", 112, 8, 0x001C, 286},
};

inline constexpr Field kFields_04F7[] = {
    {"SelectButton", 72, 1, 0x0104, 3733},
    {"SelectionSlot", 73, 1, 0x0000, 1344},
};

inline constexpr Field kFields_04F8[] = {
    {"ShotButton", 80, 1, 0x0104, 3740},
    {"MolotovButton", 81, 1, 0x0104, 3741},
};

inline constexpr Field kFields_04FA[] = {
    {"EffectName", 32, 8, 0x0018, 0},
};

inline constexpr Field kFields_04FB[] = {
    {"VehicleType", 24, 1, 0x0104, 3757},
};

inline constexpr Field kFields_04FC[] = {
    {"Effect", 0, 8, 0x001C, 46},
    {"Filter", 8, 1, 0x0104, 3758},
    {"Condition", 9, 1, 0x0204, 3759},
    {"Range", 12, 4, 0x0008, 10616},
};

inline constexpr Field kFields_04FD[] = {
    {"Immunity", 0, 8, 0x0010, 0},
    {"Duration", 8, 4, 0x0008, 10617},
};

inline constexpr Field kFields_04FE[] = {
    {"Required", 0, 2, 0x0204, 3760},
    {"Rejected", 2, 2, 0x0204, 3761},
};

inline constexpr Field kFields_04FF[] = {
    {"MyAttribute", 0, 8, 0x0010, 0},
    {"MyStat", 8, 8, 0x0010, 0},
    {"AttackerAttribute", 16, 8, 0x0010, 0},
    {"AttackerStat", 24, 8, 0x0010, 0},
    {"TargetAttribute", 32, 8, 0x0010, 0},
    {"TargetStat", 40, 8, 0x0010, 0},
    {"PlayerAttribute", 48, 8, 0x0010, 0},
    {"PlayerStat", 56, 8, 0x0010, 0},
    {"VictimAttribute", 64, 8, 0x0010, 0},
    {"VictimStat", 72, 8, 0x0010, 0},
    {"Value", 80, 4, 0x0008, 10618},
    {"MinValue", 84, 4, 0x0008, 10619},
    {"MaxValue", 88, 4, 0x0008, 10620},
    {"Condition", 92, 1, 0x0104, 3762},
    {"FloorValues", 93, 1, 0x0014, 2489},
};

inline constexpr Field kFields_0500[] = {
    {"CheckAttribute", 0, 12, 0x0024, 652},
};

inline constexpr Field kFields_0501[] = {
    {"PropStatus", 0, 1, 0x0204, 3763},
    {"PropType", 8, 8, 0x0010, 0},
    {"ObjectName", 16, 8, 0x0010, 0},
};

inline constexpr Field kFields_0502[] = {
    {"Move", 0, 8, 0x001C, 1283},
    {"When", 8, 8, 0x0030, 65535},
    {"On", 16, 2, 0x0008, 10621},
    {"Off", 18, 2, 0x0008, 10622},
    {"StackFlags", 20, 1, 0x0204, 3764},
};

inline constexpr Field kFields_0503[] = {
    {"BranchList", 0, 12, 0x0024, 653},
    {"Flags", 12, 4, 0x0204, 3765},
    {"CollisionList", 16, 12, 0x0024, 654},
    {"AllowMultipleInstances", 28, 4, 0x0000, 1363},
    {"ActionList", 32, 12, 0x0024, 655},
    {"PlaybackSpeed", 44, 2, 0x0008, 10623},
    {"TweenTime", 46, 2, 0x0008, 10624},
    {"Stacks", 48, 12, 0x0024, 656},
    {"FeatherTweenOut", 60, 2, 0x0008, 10625},
    {"MoveFeatherTweenOutOverride", 62, 2, 0x0008, 10626},
    {"OverrideBranches", 64, 0, 0x0028, 41},
    {"PhaseMatch", 76, 2, 0x0008, 10627},
    {"Duration", 78, 2, 0x0008, 10628},
    {"ImpulseOverride", 80, 8, 0x001C, 393},
    {"AnimName", 88, 8, 0x0010, 0},
    {"SubStateMachineName", 96, 8, 0x0010, 0},
    {"MoveName", 104, 8, 0x0010, 773},
    {"ReplaceMoveGroup", 112, 8, 0x0010, 0},
    {"EasingCurve", 120, 2, 0x0104, 3766},
    {"RootMotion", 122, 1, 0x0104, 3767},
    {"AnimLayer", 123, 1, 0x0000, 1364},
    {"MatchAllFeathers", 124, 1, 0x0014, 2490},
};

inline constexpr Field kFields_0504[] = {
    {"MoveSystemName", 0, 8, 0x0010, 774},
    {"MoveList", 8, 12, 0x0024, 657},
    {"BranchList", 24, 12, 0x0024, 658},
    {"CustomSynchList", 40, 12, 0x0024, 659},
    {"DefaultNavStack", 56, 12, 0x0024, 660},
    {"pointer_12", 72, 8, 0x001C, 1282},
    {"DefaultCombatStack", 80, 12, 0x0024, 661},
    {"pointer_13", 96, 8, 0x001C, 1282},
    {"DefaultStack", 104, 8, 0x001C, 1282},
    {"DefaultMove", 112, 8, 0x001C, 1286},
};

inline constexpr Field kFields_0505[] = {
    {"Move", 0, 8, 0x001C, 1283},
    {"When", 8, 8, 0x0030, 65535},
    {"LuaEvent", 16, 8, 0x0010, 0},
    {"On", 24, 2, 0x0008, 10629},
    {"Off", 26, 2, 0x0008, 10630},
    {"Type", 28, 1, 0x0105, 3768},
    {"CacheCurrentMovePos", 29, 1, 0x0014, 2491},
    {"UseCachedMovePos", 30, 1, 0x0014, 2492},
    {"Debug", 31, 1, 0x0014, 2493},
};

inline constexpr Field kFields_0506[] = {
    {"Flags", 32, 4, 0x0204, 3770},
    {"ControlDown", 36, 4, 0x0104, 3771},
    {"ControlUp", 40, 4, 0x0104, 3772},
    {"Priority", 44, 2, 0x0000, 1365},
    {"MoveID", 46, 2, 0x0000, 1366},
    {"HoldDownDelayedTime", 48, 2, 0x0008, 10633},
    {"StartPos", 50, 2, 0x0008, 10634},
    {"PhaseMatchOverride", 52, 2, 0x0008, 10635},
    {"TweenTimeOverride", 54, 2, 0x0008, 10636},
    {"FeatherTweenTimeOverride", 56, 2, 0x0008, 10637},
    {"Chance", 58, 2, 0x0008, 10638},
    {"JoystickIntent", 60, 2, 0x0008, 10639},
    {"JoystickDeadZone", 62, 2, 0x0008, 10640},
    {"MultiKeyDelayBetween", 64, 2, 0x0008, 10641},
    {"Event", 66, 2, 0x0104, 3773},
    {"RecentUpDownWindow", 68, 1, 0x0000, 1367},
    {"InputFlags", 69, 1, 0x0204, 3774},
    {"PruneFlags", 70, 1, 0x0204, 3775},
    {"EventMod", 71, 1, 0x0104, 3776},
    {"Joystick", 72, 1, 0x0104, 3777},
    {"InheritTweenTime", 73, 1, 0x0014, 2497},
    {"MatchDistance", 74, 1, 0x0014, 2498},
    {"MatchRandomPin", 75, 1, 0x0014, 2499},
    {"BlockRandomPin", 76, 1, 0x0014, 2500},
    {"PreventInteraction", 77, 1, 0x0014, 2501},
    {"MatchFeatherTweenTime", 78, 1, 0x0014, 2502},
};

inline constexpr Field kFields_0507[] = {
    {"DontOverrideFlags", 80, 1, 0x0204, 3787},
    {"ControllerSetting", 88, 8, 0x0010, 0},
    {"AltSwipeControl", 96, 8, 0x0010, 0},
};

inline constexpr Field kFields_0508[] = {
    {"ImmediateReplaceBranches", 80, 12, 0x0024, 662},
    {"LocoStartSpeed", 92, 2, 0x0008, 10664},
    {"CombineLayersToSinglePose", 94, 1, 0x0014, 2521},
};

inline constexpr Field kFields_0509[] = {
    {"BranchList", 0, 12, 0x0024, 663},
};

inline constexpr Field kFields_050A[] = {
    {"BlockBranchList", 32, 8, 0x001C, 1289},
};

inline constexpr Field kFields_050B[] = {
    {"UsingPreviousPad", 80, 4, 0x0000, 1377},
    {"StopStartDeltaTime", 84, 2, 0x0008, 10678},
    {"LocoStartSpeed", 86, 2, 0x0008, 10679},
    {"EasingCurveOverride", 88, 2, 0x0104, 3807},
    {"GamepadCacheCleared", 90, 1, 0x0014, 2534},
    {"CheckAllowBranchFlag", 91, 1, 0x0014, 2535},
};

inline constexpr Field kFields_050C[] = {
    {"TemplateSymbol", 80, 8, 0x001A, 0},
    {"TraverseLinkMove", 88, 8, 0x0010, 0},
    {"TraverseLinkMove_IsNull", 96, 1, 0x0016, 2545},
};

inline constexpr Field kFields_050D[] = {
    {"EasingCurveOverride", 80, 2, 0x0104, 3826},
    {"AllowMultipleInstancesOverride", 82, 1, 0x0000, 1384},
};

inline constexpr Field kFields_050E[] = {
    {"PromptOffset", 80, 0, 0x002C, 6},
    {"OnScreenRequired", 86, 1, 0x0014, 2564},
    {"OnScreenEndPosRequiredPercent", 88, 4, 0x0008, 10716},
    {"OnScreenCollisionCheck", 92, 1, 0x0014, 2565},
    {"Invert", 93, 1, 0x0014, 2566},
    {"InvertTraversalMove", 94, 1, 0x0014, 2567},
    {"BranchOnNextFrameComplete", 95, 1, 0x0014, 2568},
    {"TraversePathVisible", 96, 1, 0x0014, 2569},
    {"OnScreenEndTransform", 97, 1, 0x0014, 2570},
};

inline constexpr Field kFields_0511[] = {
    {"EnemyStateFilter", 80, 0, 0x002C, 152},
};

inline constexpr Field kFields_0512[] = {
    {"TraversalMove", 80, 8, 0x001C, 1283},
};

inline constexpr Field kFields_0513[] = {
    {"Decision", 80, 8, 0x001C, 1043},
    {"TrueBranch", 88, 8, 0x001C, 1286},
    {"FalseBranch", 96, 8, 0x001C, 1286},
};

inline constexpr Field kFields_0514[] = {
    {"HookName", 80, 8, 0x0018, 0},
    {"OutcomeBranches", 88, 0, 0x0028, 42},
};

inline constexpr Field kFields_0515[] = {
    {"Start", 80, 1, 0x0104, 3899},
    {"End", 81, 1, 0x0104, 3900},
    {"ResultCondition", 82, 1, 0x0104, 3901},
    {"OffsetVector", 84, 0, 0x002C, 6},
    {"EndJointName", 96, 8, 0x0010, 0},
    {"StartJointName", 104, 8, 0x0010, 0},
};

inline constexpr Field kFields_0516[] = {
    {"SkuList", 80, 12, 0x0024, 664},
};

inline constexpr Field kFields_0517[] = {
    {"CharacterID", 0, 8, 0x0010, 0},
    {"BlackboardVarName", 8, 8, 0x0010, 0},
    {"BlackboardType", 16, 1, 0x0104, 3912},
    {"MoveOverride", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0518[] = {
    {"SynchCreatureMoveOverrideList", 80, 12, 0x0024, 665},
    {"SynchFlags", 92, 2, 0x0204, 3922},
    {"SynchStartPos", 94, 2, 0x0008, 10819},
    {"WallSynchStatus", 96, 8, 0x001C, 245},
    {"SynchMove", 104, 8, 0x0010, 0},
    {"BlackboardVarName", 112, 8, 0x0010, 0},
    {"BlackboardType", 120, 1, 0x0104, 3923},
};

inline constexpr Field kFields_0519[] = {
    {"Moves", 0, 12, 0x0024, 666},
};

inline constexpr Field kFields_051A[] = {
    {"Triggered", 80, 1, 0x0000, 1420},
    {"IsOnNavMesh", 81, 1, 0x0000, 1421},
    {"TimeAliveGreaterThan", 82, 2, 0x0008, 10831},
    {"TargetSpeedEqual", 84, 2, 0x0008, 10832},
    {"TargetSpeedNotEqual", 86, 2, 0x0008, 10833},
    {"WildlifeEvent", 88, 8, 0x0010, 0},
};

inline constexpr Field kFields_051B[] = {
    {"Type", 0, 1, 0x0105, 3933},
};

inline constexpr Field kFields_051C[] = {
    {"Damage", 4, 4, 0x0008, 10834},
    {"Attacker", 8, 1, 0x0104, 3935},
    {"Victim", 9, 1, 0x0104, 3936},
    {"HitFlags", 16, 8, 0x0204, 3937},
};

inline constexpr Field kFields_051D[] = {
    {"SkillTree", 8, 8, 0x0010, 0},
    {"SkillNode", 16, 8, 0x0010, 0},
    {"AdditionalBonusIndex", 24, 4, 0x0000, 1422},
};

inline constexpr Field kFields_051E[] = {
    {"CheckPlatinumToken", 1, 1, 0x0104, 3940},
    {"SkillTree", 8, 8, 0x0010, 0},
    {"SkillNode", 16, 8, 0x0010, 0},
    {"TokenOption", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_051F[] = {
    {"EquipmentName", 8, 8, 0x0010, 0},
    {"WalletName", 16, 8, 0x0010, 0},
    {"SlotSetName", 24, 8, 0x0010, 0},
    {"SlotName", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_0520[] = {
    {"FlagName", 8, 8, 0x0010, 0},
    {"WalletName", 16, 8, 0x0010, 0},
    {"SlotSetName", 24, 8, 0x0010, 0},
    {"SlotName", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_0521[] = {
    {"Direction", 1, 1, 0x0104, 3944},
    {"ZeroJointOffset", 2, 0, 0x002C, 6},
    {"EndPosOffset", 8, 0, 0x002C, 6},
    {"OnlyRunOnRoundRobin", 14, 1, 0x0014, 2661},
    {"EntityType", 16, 4, 0x0204, 3945},
    {"CollidesWith", 20, 4, 0x0204, 3946},
    {"Angle", 24, 4, 0x0008, 10841},
    {"RaycastDistance", 28, 4, 0x0008, 10842},
};

inline constexpr Field kFields_0522[] = {
    {"Direction", 1, 1, 0x0104, 3948},
    {"UsePlayerForCheck", 2, 1, 0x0014, 2662},
    {"FilterData", 8, 0, 0x002C, 247},
};

inline constexpr Field kFields_0523[] = {
    {"UseCurrentGroundPos", 1, 1, 0x0014, 2663},
    {"UseLastGroundPos", 2, 1, 0x0014, 2664},
    {"ReturnTrueIfEitherOffNavmesh", 3, 1, 0x0014, 2665},
    {"XZTolerance", 4, 4, 0x0008, 10845},
    {"YTolerance", 8, 4, 0x0008, 10846},
};

inline constexpr Field kFields_0524[] = {
    {"ObjectNearby", 0, 0, 0x002C, 1314},
    {"EquipmentQuery", 104, 0, 0x002C, 1311},
    {"EquipmentFlagQuery", 144, 0, 0x002C, 1312},
    {"SkillTreeAdditionalBonusQuery", 184, 0, 0x002C, 1309},
    {"SkillTreeTokenQuery", 216, 0, 0x002C, 1310},
    {"NearbyCollisionRaycast", 248, 0, 0x002C, 1313},
    {"HealthAfterSimulatedDamage", 280, 0, 0x002C, 1308},
    {"TargetReachableWithoutTraverseLinkAdvanced", 304, 0, 0x002C, 1315},
    {"SlotTags", 316, 4, 0x0204, 3972},
    {"EnemyID", 320, 8, 0x0010, 0},
    {"CombatTargetID", 328, 8, 0x0010, 0},
    {"SelfID", 336, 8, 0x0010, 0},
    {"PrimaryCompanionID", 344, 8, 0x0010, 0},
    {"SelfContext", 352, 8, 0x0010, 0},
    {"EnemyContext", 360, 8, 0x0010, 0},
    {"SelfMarkerID", 368, 8, 0x0010, 0},
    {"EnemyMarkerID", 376, 8, 0x0010, 0},
    {"WeaponSurfaceMaterial", 384, 8, 0x0010, 0},
    {"WeaponMarkerID", 392, 8, 0x0010, 0},
    {"EnemyWeaponSurfaceMaterial", 400, 8, 0x0010, 0},
    {"EnemyWeaponMarkerID", 408, 8, 0x0010, 0},
    {"ExposedSideName", 416, 8, 0x0010, 0},
    {"MyExposedSideName", 424, 8, 0x0010, 0},
    {"EnemyExposedSideName", 432, 8, 0x0010, 0},
    {"DamageImmunity", 440, 8, 0x0010, 0},
    {"HitFlags", 448, 8, 0x0204, 3973},
    {"PartFlags", 456, 8, 0x0204, 3974},
    {"SelfDynamicFlags", 464, 8, 0x0010, 0},
    {"EnemyDynamicFlags", 472, 8, 0x0010, 0},
    {"PlayerDynamicFlags", 480, 8, 0x0010, 0},
    {"PrimaryCompanionDynamicFlags", 488, 8, 0x0010, 0},
    {"CombatTargetDynamicFlags", 496, 8, 0x0010, 0},
    {"SelectedControllerOption", 504, 8, 0x0010, 0},
    {"TraversalMove", 512, 8, 0x0010, 0},
    {"TraverseLinkMove", 520, 8, 0x0010, 0},
    {"TraverseGraphMove", 528, 8, 0x0010, 0},
    {"WalletHasEquipment", 536, 8, 0x0010, 0},
    {"WalletHasEquipmentWithFlag", 544, 8, 0x0010, 0},
    {"EquipmentEquipped", 552, 8, 0x0010, 0},
    {"EquipmentEquippedWithFlag", 560, 8, 0x0010, 0},
    {"SkillTreeNodeAcquired", 568, 8, 0x0010, 0},
    {"CreatureBlackboardString", 576, 8, 0x0010, 0},
    {"TargetBlackboardString", 584, 8, 0x0010, 0},
    {"PlayerBlackboardString", 592, 8, 0x0010, 0},
    {"GlobalBlackboardString", 600, 8, 0x0010, 0},
    {"ArrowPartFlags", 608, 8, 0x0204, 3975},
    {"ArrowEnemyID", 616, 8, 0x0010, 0},
    {"ArrowSurfaceMaterial", 624, 8, 0x0010, 0},
    {"WeaponLevel", 632, 4, 0x0000, 1424},
    {"WeaponAmmo", 636, 4, 0x0000, 1425},
    {"WeaponDistance", 640, 4, 0x0008, 10860},
    {"NumEmbeddedInTarget", 644, 4, 0x0000, 1426},
    {"NumDetonatableSpearsInTotal", 648, 4, 0x0000, 1427},
    {"NumDetonatableSpearsInCreature", 652, 4, 0x0000, 1428},
    {"DistanceToClosestDetonatableSpear", 656, 4, 0x0008, 10861},
    {"DistanceToClosestDetonatableSpearInCreature", 660, 4, 0x0008, 10862},
    {"EnemyWeaponLevel", 664, 4, 0x0000, 1429},
    {"EnemyWeaponAmmo", 668, 4, 0x0000, 1430},
    {"EnemyWeaponDistance", 672, 4, 0x0008, 10863},
    {"ResourceLevel", 676, 4, 0x0000, 1431},
    {"MeterLevel", 680, 4, 0x0008, 10864},
    {"MeterPercent", 684, 4, 0x0008, 10865},
    {"MeterMin", 688, 4, 0x0008, 10866},
    {"MeterMax", 692, 4, 0x0008, 10867},
    {"EnemyMeterLevel", 696, 4, 0x0008, 10868},
    {"EnemyMeterPercent", 700, 4, 0x0008, 10869},
    {"EnemyMeterMin", 704, 4, 0x0008, 10870},
    {"EnemyMeterMax", 708, 4, 0x0008, 10871},
    {"PlayerMeterLevel", 712, 4, 0x0008, 10872},
    {"PlayerMeterPercent", 716, 4, 0x0008, 10873},
    {"PlayerMeterMin", 720, 4, 0x0008, 10874},
    {"PlayerMeterMax", 724, 4, 0x0008, 10875},
    {"MyAttribute", 728, 4, 0x0008, 10876},
    {"AttackerAttribute", 732, 4, 0x0008, 10877},
    {"TargetAttribute", 736, 4, 0x0008, 10878},
    {"PlayerAttribute", 740, 4, 0x0008, 10879},
    {"VictimAttribute", 744, 4, 0x0008, 10880},
    {"PlayerAttributePowerLevel", 748, 4, 0x0008, 10881},
    {"MyStat", 752, 4, 0x0008, 10882},
    {"AttackerStat", 756, 4, 0x0008, 10883},
    {"TargetStat", 760, 4, 0x0008, 10884},
    {"PlayerStat", 764, 4, 0x0008, 10885},
    {"VictimStat", 768, 4, 0x0008, 10886},
    {"MyShieldValue", 772, 4, 0x0008, 10887},
    {"EnemyShieldValue", 776, 4, 0x0008, 10888},
    {"Tags", 780, 4, 0x0204, 3976},
    {"SelfTags", 784, 4, 0x0204, 3977},
    {"EnemyTags", 788, 4, 0x0204, 3978},
    {"ControlDown", 792, 4, 0x0204, 3979},
    {"ControlUp", 796, 4, 0x0204, 3980},
    {"DistanceFromLastLedge", 800, 4, 0x0008, 10889},
    {"MoodFlags", 804, 4, 0x0000, 1432},
    {"DistanceToTraversePathMove", 808, 4, 0x0008, 10890},
    {"DistanceToTraverseEnter", 812, 4, 0x0008, 10891},
    {"FacingToObjectTolerance", 816, 4, 0x0008, 10892},
    {"FacingToDirectionTolerance", 820, 4, 0x0008, 10893},
    {"InFrontOfObjectAngleTolerance", 824, 4, 0x0008, 10894},
    {"TraverseGraphPlayerNodeDifference", 828, 4, 0x0000, 1433},
    {"FlumeCollisionReaction", 832, 4, 0x0000, 1434},
    {"WolfSledHardStopStage", 836, 4, 0x0000, 1435},
    {"EquipmentTrait", 840, 4, 0x0008, 10895},
    {"CreatureBlackboardFloat", 844, 4, 0x0008, 10896},
    {"CreatureBlackboardInt", 848, 4, 0x0000, 1436},
    {"TargetBlackboardFloat", 852, 4, 0x0008, 10897},
    {"TargetBlackboardInt", 856, 4, 0x0000, 1437},
    {"PlayerBlackboardFloat", 860, 4, 0x0008, 10898},
    {"PlayerBlackboardInt", 864, 4, 0x0000, 1438},
    {"GlobalBlackboardFloat", 868, 4, 0x0008, 10899},
    {"GlobalBlackboardInt", 872, 4, 0x0000, 1439},
    {"StickAngleToInteractHorizontal", 876, 4, 0x0008, 10900},
    {"StickAngleToInteractVertical", 880, 4, 0x0008, 10901},
    {"CameraAngleToInteractHorizontal", 884, 4, 0x0008, 10902},
    {"CameraAngleToInteractVertical", 888, 4, 0x0008, 10903},
    {"StickAngleToTraverseLinkEdgeHorizontal", 892, 4, 0x0008, 10904},
    {"StickAngleToTraverseLinkEdgeVertical", 896, 4, 0x0008, 10905},
    {"CameraAngleToTraverseLinkEdgeHorizontal", 900, 4, 0x0008, 10906},
    {"CameraAngleToTraverseLinkEdgeVertical", 904, 4, 0x0008, 10907},
    {"CameraAngleToTargetInteractHorizontal", 908, 4, 0x0008, 10908},
    {"CameraAngleToTargetInteractVertical", 912, 4, 0x0008, 10909},
    {"StickAngleToTargetInteractHorizontal", 916, 4, 0x0008, 10910},
    {"StickAngleToTargetInteractVertical", 920, 4, 0x0008, 10911},
    {"ArrowCollisionID", 924, 4, 0x0000, 1440},
    {"UISettingSubtitleBGAlpha", 928, 4, 0x0008, 10912},
    {"UISettingCaptionBGAlpha", 932, 4, 0x0008, 10913},
    {"UISettingPuzzleAssistance", 936, 4, 0x0000, 1441},
    {"UISettingMotionReduction", 940, 4, 0x0000, 1442},
    {"UISettingRepeatedButtonPressChoice", 944, 4, 0x0000, 1443},
    {"UISettingSubtitleSpeakerPolicy", 948, 4, 0x0000, 1444},
    {"UISettingRobustTutorialMode", 952, 4, 0x0000, 1445},
    {"UISettingTutorialVariance", 956, 4, 0x0000, 1446},
    {"UISettingAccessibilityDot", 960, 4, 0x0000, 1447},
    {"UISettingAccessibilityDotColor", 964, 4, 0x0000, 1448},
    {"UISettingSubtitleSize", 968, 4, 0x0000, 1449},
    {"UISettingControllerVisualizationOpacity", 972, 4, 0x0000, 1450},
    {"UISettingCaptionSize", 976, 4, 0x0000, 1451},
    {"UISettingAutoSprintDelay", 980, 4, 0x0000, 1452},
    {"UISettingTraversalAssistance", 984, 4, 0x0000, 1453},
    {"UISettingUserLockOn", 988, 4, 0x0000, 1454},
    {"UISettingColorCorrection", 992, 4, 0x0000, 1455},
    {"UISettingHighContrastDisplay", 996, 4, 0x0000, 1456},
    {"UISettingHeroContrast", 1000, 4, 0x0000, 1457},
    {"UISettingEnemyContrast", 1004, 4, 0x0000, 1458},
    {"UISettingEnviromentMarkingContrast", 1008, 4, 0x0000, 1459},
    {"UISettingBackgroundContrast", 1012, 4, 0x0000, 1460},
    {"UISettingAccessibilityHighlightLightingDesaturation", 1016, 4, 0x0008, 10914},
    {"UISettingScreenFilterColor", 1020, 4, 0x0000, 1461},
    {"UISettingScreenFilterIntensity", 1024, 4, 0x0000, 1462},
    {"UISettingVisionAssistance", 1028, 4, 0x0000, 1463},
    {"UISettingCaptionsHearingAssistance", 1032, 4, 0x0000, 1464},
    {"UISettingMotorAssistance", 1036, 4, 0x0000, 1465},
    {"UISettingPauseMenuTouchpadEntry", 1040, 4, 0x0000, 1466},
    {"UISettingCameraAssist", 1044, 4, 0x0000, 1467},
    {"UISettingMiniBossCheckpoints", 1048, 4, 0x0000, 1468},
    {"UISettingPuzzleZoomSnap", 1052, 4, 0x0000, 1469},
    {"UISettingSubtitlesOn", 1056, 4, 0x0000, 1470},
    {"UISettingSoundCaptionsOn", 1060, 4, 0x0000, 1471},
    {"UISettingAutoPickup", 1064, 4, 0x0000, 1472},
    {"UISettingEnhancedRoll", 1068, 4, 0x0000, 1473},
    {"UISettingHighContrastCompanion", 1072, 4, 0x0000, 1474},
    {"UISettingHighContrastHazard", 1076, 4, 0x0000, 1475},
    {"UISettingHighContrastBoss", 1080, 4, 0x0000, 1476},
    {"UISettingHighContrastInteractive", 1084, 4, 0x0000, 1477},
    {"UISettingHighContrastNPC", 1088, 4, 0x0000, 1478},
    {"UISettingQteType", 1092, 4, 0x0000, 1479},
    {"UISettingMotionSensorFunctionAiming", 1096, 4, 0x0000, 1480},
    {"UISettingMotionSensorFunctionVerticalSensitivity", 1100, 4, 0x0000, 1481},
    {"UISettingMotionSensorFunctionHorizonalSensitivity", 1104, 4, 0x0000, 1482},
    {"UISettingAudioCueVolume", 1108, 4, 0x0000, 1483},
    {"UISettingAttackIndicatorColorIndex", 1112, 4, 0x0000, 1484},
    {"CurrentControlSettingIndex", 1116, 4, 0x0000, 1485},
    {"DiceRoll", 1120, 2, 0x0008, 10915},
    {"DiceRollHit", 1122, 2, 0x0008, 10916},
    {"WeaponThrowStatus", 1124, 2, 0x0204, 3981},
    {"EnemyWeaponThrowStatus", 1126, 2, 0x0204, 3982},
    {"RagdollStatusFlags", 1128, 2, 0x0204, 3983},
    {"EnemyRagdollStatusFlags", 1130, 2, 0x0204, 3984},
    {"ModeFlags", 1132, 2, 0x0204, 3985},
    {"RagdollHitFlags", 1134, 2, 0x0204, 3986},
    {"PhaseValidity", 1136, 2, 0x0000, 1486},
    {"EnemyPhaseValidity", 1138, 2, 0x0000, 1487},
    {"EventType", 1140, 2, 0x0104, 3987},
    {"InteractAboveCreatureAngle", 1142, 2, 0x0008, 10917},
    {"InteractBelowCreatureAngle", 1144, 2, 0x0008, 10918},
    {"InteractRightOfCreatureAngle", 1146, 2, 0x0008, 10919},
    {"InteractLeftOfCreatureAngle", 1148, 2, 0x0008, 10920},
    {"MinWalkHeightForLedgeCatch", 1150, 2, 0x0008, 10921},
    {"MinSprintHeightForPrefall", 1152, 2, 0x0008, 10922},
    {"MinSprintHeightForLedgeCatch", 1154, 2, 0x0008, 10923},
    {"ReticlePitch", 1156, 2, 0x0008, 10924},
    {"CameraFacingDelta", 1158, 2, 0x0008, 10925},
    {"CameraFacingDeltaDirectional", 1160, 2, 0x0008, 10926},
    {"IntentFacingDeltaDirectional", 1162, 2, 0x0008, 10927},
    {"StickAngleChange", 1164, 2, 0x0008, 10928},
    {"StickIntentChange", 1166, 2, 0x0008, 10929},
    {"CameraAngleChange", 1168, 2, 0x0008, 10930},
    {"StickAngle", 1170, 2, 0x0008, 10931},
    {"StickAngleInput", 1172, 2, 0x0008, 10932},
    {"LeftStickAngleAbsolute", 1174, 2, 0x0008, 10933},
    {"RightStickAngleAbsolute", 1176, 2, 0x0008, 10934},
    {"StickDeflectionTime", 1178, 2, 0x0008, 10935},
    {"StickIdleTime", 1180, 2, 0x0008, 10936},
    {"StickDeflection", 1182, 2, 0x0008, 10937},
    {"StickVelocity", 1184, 2, 0x0008, 10938},
    {"StickIntent", 1186, 2, 0x0008, 10939},
    {"StickIntentDamped", 1188, 2, 0x0008, 10940},
    {"RightStickIntent", 1190, 2, 0x0008, 10941},
    {"TargetSpeed", 1192, 2, 0x0008, 10942},
    {"MaxSpeedOverride", 1194, 2, 0x0008, 10943},
    {"WalkSpeed", 1196, 2, 0x0008, 10944},
    {"JogSpeed", 1198, 2, 0x0008, 10945},
    {"RunSpeed", 1200, 2, 0x0008, 10946},
    {"SprintSpeed", 1202, 2, 0x0008, 10947},
    {"DistanceToLedgeBarrier", 1204, 2, 0x0008, 10948},
    {"LedgeBarrierNormalAngle", 1206, 2, 0x0008, 10949},
    {"LedgeBarrierNormalCameraAngle", 1208, 2, 0x0008, 10950},
    {"FallDistanceAfterLedgeBarrier_Persist", 1210, 2, 0x0008, 10951},
    {"DistanceToCollision", 1212, 2, 0x0008, 10952},
    {"SlopeAngle", 1214, 2, 0x0008, 10953},
    {"SlopeFaceAngle", 1216, 2, 0x0008, 10954},
    {"StopStartTurnAngle", 1218, 2, 0x0008, 10955},
    {"Velocity", 1220, 2, 0x0008, 10956},
    {"VerticalVelocity", 1222, 2, 0x0008, 10957},
    {"TraverseLinkEdgeAngle", 1224, 2, 0x0008, 10958},
    {"TraverseLinkHeight", 1226, 2, 0x0008, 10959},
    {"TraverseLinkWarpHeight", 1228, 2, 0x0008, 10960},
    {"TraverseLinkDistanceToEnd", 1230, 2, 0x0008, 10961},
    {"TraverseLinkDistanceToEndXZ", 1232, 2, 0x0008, 10962},
    {"TraverseLinkDistanceToEdge", 1234, 2, 0x0008, 10963},
    {"TraverseLinkDistanceToEdgeXZ", 1236, 2, 0x0008, 10964},
    {"DistanceToGround", 1238, 2, 0x0008, 10965},
    {"TargetDistanceToGround", 1240, 2, 0x0008, 10966},
    {"FallDistance", 1242, 2, 0x0008, 10967},
    {"AngleToTargetHorizontal", 1244, 2, 0x0008, 10968},
    {"AngleToTargetVertical", 1246, 2, 0x0008, 10969},
    {"AngleToTargetFacingHorizontal", 1248, 2, 0x0008, 10970},
    {"AngleToTargetExposedSideJointHorizontal", 1250, 2, 0x0008, 10971},
    {"AngleToPlayerHorizontal", 1252, 2, 0x0008, 10972},
    {"AngleToPlayerVertical", 1254, 2, 0x0008, 10973},
    {"AngleToPlayerFacingHorizontal", 1256, 2, 0x0008, 10974},
    {"EnemyAngleToMeHorizontal", 1258, 2, 0x0008, 10975},
    {"EnemyAngleToMeVertical", 1260, 2, 0x0008, 10976},
    {"DistanceToTarget", 1262, 2, 0x0008, 10977},
    {"DistanceToTargetXZ", 1264, 2, 0x0008, 10978},
    {"DistanceToTargetY", 1266, 2, 0x0008, 10979},
    {"DistanceToTargetYSigned", 1268, 2, 0x0008, 10980},
    {"DistanceToTargetCapsule", 1270, 2, 0x0008, 10981},
    {"DistanceToTargetCapsuleXZ", 1272, 2, 0x0008, 10982},
    {"DistanceToInteractJointOverride", 1274, 2, 0x0008, 10983},
    {"DistanceToPlayer", 1276, 2, 0x0008, 10984},
    {"DistanceToPlayerXZ", 1278, 2, 0x0008, 10985},
    {"DistanceToPlayerY", 1280, 2, 0x0008, 10986},
    {"DistanceToPlayerYSigned", 1282, 2, 0x0008, 10987},
    {"DistanceToPrimaryCompanion", 1284, 2, 0x0008, 10988},
    {"DistanceToPrimaryCompanionXZ", 1286, 2, 0x0008, 10989},
    {"DistanceToPrimaryCompanionY", 1288, 2, 0x0008, 10990},
    {"DistanceToPrimaryCompanionYSigned", 1290, 2, 0x0008, 10991},
    {"DistanceFromTargetToPlayer", 1292, 2, 0x0008, 10992},
    {"DistanceFromTargetToPlayerXZ", 1294, 2, 0x0008, 10993},
    {"DistanceFromTargetToPlayerY", 1296, 2, 0x0008, 10994},
    {"DistanceFromTargetToPlayerYSigned", 1298, 2, 0x0008, 10995},
    {"FocusAngle", 1300, 2, 0x0008, 10996},
    {"TargetAngle", 1302, 2, 0x0008, 10997},
    {"PreviousPadCurrentPadAngle", 1304, 2, 0x0008, 10998},
    {"AbsRawTargetDirAngle", 1306, 2, 0x0008, 10999},
    {"PathLength", 1308, 2, 0x0008, 11000},
    {"PathStraightDistance", 1310, 2, 0x0008, 11001},
    {"PathAngle", 1312, 2, 0x0008, 11002},
    {"AbsPathAngle", 1314, 2, 0x0008, 11003},
    {"PathfindStopTargetXDistance", 1316, 2, 0x0008, 11004},
    {"PositionToFocusAngle", 1318, 2, 0x0008, 11005},
    {"LocoStrafeDir", 1320, 2, 0x0104, 3988},
    {"DistanceToFlightVolumeXZ", 1322, 2, 0x0008, 11006},
    {"DistanceToFlightVolumeXYZ", 1324, 2, 0x0008, 11007},
    {"HeightOverFlightVolume", 1326, 2, 0x0008, 11008},
    {"CombinedStickAngle", 1328, 2, 0x0008, 11009},
    {"CombinedStickIntent", 1330, 2, 0x0008, 11010},
    {"BoatLinearVelocity", 1332, 2, 0x0008, 11011},
    {"WolfSledStickIntent", 1334, 2, 0x0008, 11012},
    {"WolfSledLinearVelocity", 1336, 2, 0x0008, 11013},
    {"YakStickIntent", 1338, 2, 0x0008, 11014},
    {"IsPlayer", 1340, 1, 0x0014, 2671},
    {"IsPlayerAtreus", 1341, 1, 0x0014, 2672},
    {"IsHostileToPlayer", 1342, 1, 0x0014, 2673},
    {"IsHighFidelity", 1343, 1, 0x0014, 2674},
    {"IsTargetHighFidelity", 1344, 1, 0x0014, 2675},
    {"SlotStage", 1345, 1, 0x0000, 1488},
    {"SlotState", 1346, 1, 0x0104, 3989},
    {"PickupStage", 1347, 1, 0x0000, 1489},
    {"PickupState", 1348, 1, 0x0204, 3990},
    {"EnemyPickupStage", 1349, 1, 0x0000, 1490},
    {"EnemyPickupState", 1350, 1, 0x0204, 3991},
    {"PlayerPickupStage", 1351, 1, 0x0000, 1491},
    {"PlayerPickupState", 1352, 1, 0x0204, 3992},
    {"HasWeapon", 1353, 1, 0x0014, 2676},
    {"WeaponState", 1354, 1, 0x0104, 3993},
    {"WeaponStowed", 1355, 1, 0x0014, 2677},
    {"WeaponActive", 1356, 1, 0x0014, 2678},
    {"WeaponActiveAny", 1357, 1, 0x0014, 2679},
    {"WeaponInactiveAny", 1358, 1, 0x0014, 2680},
    {"WeaponCinematic", 1359, 1, 0x0014, 2681},
    {"WeaponThrown", 1360, 1, 0x0014, 2682},
    {"WeaponStuckState", 1361, 1, 0x0104, 3994},
    {"WeaponCanThrowAtTarget", 1362, 1, 0x0014, 2683},
    {"WeaponEmbeddedInMe", 1363, 1, 0x0014, 2684},
    {"WeaponWaitingToRespawn", 1364, 1, 0x0014, 2685},
    {"ThrowableCaughtByMe", 1365, 1, 0x0014, 2686},
    {"EnemyHasWeapon", 1366, 1, 0x0014, 2687},
    {"EnemyWeaponState", 1367, 1, 0x0104, 3995},
    {"EnemyWeaponStowed", 1368, 1, 0x0014, 2688},
    {"EnemyWeaponActive", 1369, 1, 0x0014, 2689},
    {"EnemyWeaponActiveAny", 1370, 1, 0x0014, 2690},
    {"EnemyWeaponInactiveAny", 1371, 1, 0x0014, 2691},
    {"EnemyWeaponCinematic", 1372, 1, 0x0014, 2692},
    {"EnemyWeaponThrown", 1373, 1, 0x0014, 2693},
    {"EnemyWeaponStuckState", 1374, 1, 0x0104, 3996},
    {"EnemyWeaponEmbeddedInMe", 1375, 1, 0x0014, 2694},
    {"EnemyWeaponWaitingToRespawn", 1376, 1, 0x0014, 2695},
    {"HasMaxPaintTargets", 1377, 1, 0x0014, 2696},
    {"PropStatus", 1378, 1, 0x0204, 3997},
    {"HasMeter", 1379, 1, 0x0014, 2697},
    {"EnemyHasMeter", 1380, 1, 0x0014, 2698},
    {"PlayerHasMeter", 1381, 1, 0x0014, 2699},
    {"InCombat", 1382, 1, 0x0014, 2700},
    {"MyMentalState", 1383, 1, 0x0104, 3998},
    {"InSynch", 1384, 1, 0x0014, 2701},
    {"EnemyInSynch", 1385, 1, 0x0014, 2702},
    {"IsDying", 1386, 1, 0x0014, 2703},
    {"IsDead", 1387, 1, 0x0014, 2704},
    {"IsPlayerDying", 1388, 1, 0x0014, 2705},
    {"DeathCausedByDeathPlaneOrFalling", 1389, 1, 0x0014, 2706},
    {"IsInvulnerable", 1390, 1, 0x0014, 2707},
    {"IsTargetInvulnerable", 1391, 1, 0x0014, 2708},
    {"HasPendingRagdollHitFlags", 1392, 1, 0x0014, 2709},
    {"Difficulty", 1393, 1, 0x0204, 3999},
    {"HealthThreshold", 1394, 1, 0x0000, 1492},
    {"InteractionType", 1395, 1, 0x0104, 4000},
    {"IsInteracting", 1396, 1, 0x0014, 2710},
    {"IsInSyncedSequence", 1397, 1, 0x0014, 2711},
    {"IsInCinematicSyncedSequence", 1398, 1, 0x0014, 2712},
    {"CameraAtOrbitConstraint", 1399, 1, 0x0014, 2713},
    {"CameraAimAssistMode", 1400, 1, 0x0000, 1493},
    {"HasTarget", 1401, 1, 0x0014, 2714},
    {"IsTargetHostile", 1402, 1, 0x0014, 2715},
    {"IsTargetOnScreen", 1403, 1, 0x0014, 2716},
    {"IsTargetOnGround", 1404, 1, 0x0014, 2717},
    {"IsSelfOnScreen", 1405, 1, 0x0014, 2718},
    {"IsTargetTargetingMe", 1406, 1, 0x0014, 2719},
    {"IsPlayerTargetingMe", 1407, 1, 0x0014, 2720},
    {"IsTargetInAir", 1408, 1, 0x0014, 2721},
    {"TargetMatchesReticleTarget", 1409, 1, 0x0014, 2722},
    {"TargetMatchesPlayerLockOnTarget", 1410, 1, 0x0014, 2723},
    {"TargetMatchesCenterOfScreenEnemy", 1411, 1, 0x0014, 2724},
    {"HasSpline", 1412, 1, 0x0014, 2725},
    {"HasContextAction", 1413, 1, 0x0014, 2726},
    {"OnGround", 1414, 1, 0x0014, 2727},
    {"FoundLedgeBarrier", 1415, 1, 0x0014, 2728},
    {"FoundObstacleAfterLedgeBarrier_Persist", 1416, 1, 0x0014, 2729},
    {"StriderHangSharedPoint", 1417, 1, 0x0014, 2730},
    {"IsOnStriderHang", 1418, 1, 0x0014, 2731},
    {"PlayerQuickTurnDirectionType", 1419, 1, 0x0104, 4001},
    {"IsInBranchWindow", 1420, 1, 0x0014, 2732},
    {"IsInBranchWindowFirstFrame", 1421, 1, 0x0014, 2733},
    {"IsInEarlyBranchWindow", 1422, 1, 0x0014, 2734},
    {"HasMotionWarpEnded", 1423, 1, 0x0014, 2735},
    {"UsingPreviousPad", 1424, 1, 0x0014, 2736},
    {"IsPathingEnabled", 1425, 1, 0x0014, 2737},
    {"IsTraversalApproachEnabled", 1426, 1, 0x0014, 2738},
    {"IsPlayerTraversalApproachEnabled", 1427, 1, 0x0014, 2739},
    {"InBtreeApproach", 1428, 1, 0x0014, 2740},
    {"InBtreeApproachWithStop", 1429, 1, 0x0014, 2741},
    {"InTemporaryStop", 1430, 1, 0x0014, 2742},
    {"IsReadyToStopBtreeApproach", 1431, 1, 0x0014, 2743},
    {"IsStartingApproach", 1432, 1, 0x0014, 2744},
    {"PlantOnApproachStart", 1433, 1, 0x0014, 2745},
    {"CloseRangeApproach", 1434, 1, 0x0104, 4002},
    {"TraverseGraphMovingAwayFromPlayer", 1435, 1, 0x0014, 2746},
    {"TargetReachableWithoutTraverseLink", 1436, 1, 0x0014, 2747},
    {"PlayerReachableWithoutTraverseLink", 1437, 1, 0x0014, 2748},
    {"IsTargetOnNavmesh", 1438, 1, 0x0014, 2749},
    {"IsPlayerOnNavmesh", 1439, 1, 0x0014, 2750},
    {"IsPlayerFunneling", 1440, 1, 0x0014, 2751},
    {"ValidFlightVolume", 1441, 1, 0x0014, 2752},
    {"MovementLimitedByCurrentAOO", 1442, 1, 0x0014, 2753},
    {"HasAOO", 1443, 1, 0x0014, 2754},
    {"MyPositionInMyAOO", 1444, 1, 0x0014, 2755},
    {"AOOAssignmentType", 1445, 1, 0x0104, 4003},
    {"AnimExists", 1446, 1, 0x0014, 2756},
    {"BoatCollidingWithEnvironment", 1447, 1, 0x0014, 2757},
    {"BoatDirectionalMapping", 1448, 1, 0x0104, 4004},
    {"WolfSledGearMode", 1449, 1, 0x0104, 4005},
    {"WolfSledCreatureType", 1450, 1, 0x0104, 4006},
    {"WolfSledWolfAvoidanceState", 1451, 1, 0x0104, 4007},
    {"IsPlayerOnWolfSled", 1452, 1, 0x0014, 2758},
    {"IsCompanionOnWolfSled", 1453, 1, 0x0014, 2759},
    {"IsWolfSledInDrift", 1454, 1, 0x0014, 2760},
    {"IsPlayerOnYak", 1455, 1, 0x0014, 2761},
    {"IsCompanionOnYak", 1456, 1, 0x0014, 2762},
    {"IsPlayerInteractingWithYak", 1457, 1, 0x0014, 2763},
    {"IsCompanionInteractingWithYak", 1458, 1, 0x0014, 2764},
    {"CreatureBlackboardValueExists", 1459, 1, 0x0014, 2765},
    {"CreatureBlackboardBoolean", 1460, 1, 0x0014, 2766},
    {"TargetBlackboardValueExists", 1461, 1, 0x0014, 2767},
    {"TargetBlackboardBoolean", 1462, 1, 0x0014, 2768},
    {"PlayerBlackboardValueExists", 1463, 1, 0x0014, 2769},
    {"PlayerBlackboardBoolean", 1464, 1, 0x0014, 2770},
    {"GlobalBlackboardValueExists", 1465, 1, 0x0014, 2771},
    {"GlobalBlackboardBoolean", 1466, 1, 0x0014, 2772},
    {"Strafe", 1467, 1, 0x0014, 2773},
    {"Align", 1468, 1, 0x0014, 2774},
    {"PhaseMatches", 1469, 1, 0x0014, 2775},
    {"GetQuestState", 1470, 1, 0x0104, 4008},
    {"InTraverseGraphZone", 1471, 1, 0x0014, 2776},
    {"InTraverseLinkZone", 1472, 1, 0x0014, 2777},
    {"IsPlayerInsideCameraZone", 1473, 1, 0x0014, 2778},
    {"IsPlayerLockedOnToMe", 1474, 1, 0x0014, 2779},
    {"IsPS5", 1475, 1, 0x0014, 2780},
    {"IsPC", 1476, 1, 0x0014, 2781},
    {"UISettingMiniGameplaySkipped", 1477, 1, 0x0014, 2782},
    {"UISettingAxeReflectVisual", 1478, 1, 0x0014, 2783},
    {"UISettingSubtitleBlur", 1479, 1, 0x0014, 2784},
    {"UISettingSubtitleDirectionIndicator", 1480, 1, 0x0014, 2785},
    {"UISettingControllerVisualization", 1481, 1, 0x0014, 2786},
    {"UISettingNavigationAssistance", 1482, 1, 0x0014, 2787},
    {"UISettingCaptionBlur", 1483, 1, 0x0014, 2788},
    {"UISettingCaptionDirectionIndicator", 1484, 1, 0x0014, 2789},
    {"UISettingAimToggle", 1485, 1, 0x0014, 2790},
    {"UISettingBlockToggle", 1486, 1, 0x0014, 2791},
    {"UISettingAudioCues", 1487, 1, 0x0014, 2792},
    {"UISettingTextToSpeech", 1488, 1, 0x0014, 2793},
    {"UISettingUserSuppressDesignerMessages", 1489, 1, 0x0014, 2794},
    {"UISettingCineSkipEnabled", 1490, 1, 0x0014, 2795},
    {"UISettingHighContrastMode", 1491, 1, 0x0014, 2796},
    {"UISettingDodgePrompt", 1492, 1, 0x0014, 2797},
    {"UISettingArcThrowVisual", 1493, 1, 0x0014, 2798},
    {"UISettingReducedFlashing", 1494, 1, 0x0014, 2799},
    {"UISettingSingleButtonEvade", 1495, 1, 0x0014, 2800},
    {"UISettingEnhancedDodge", 1496, 1, 0x0014, 2801},
    {"UISettingNeutralRoll", 1497, 1, 0x0014, 2802},
    {"UISettingHighContrastCinematics", 1498, 1, 0x0014, 2803},
    {"UISettingShieldBurstToggleAlt", 1499, 1, 0x0014, 2804},
    {"UISettingBarehandsToggle", 1500, 1, 0x0014, 2805},
    {"InBanterConversation", 1501, 1, 0x0014, 2806},
    {"InBanterConversationMainCharactersConflict", 1502, 1, 0x0014, 2807},
    {"IsAnyBanterPlaying", 1503, 1, 0x0014, 2808},
    {"IsSelfPlayingBanter", 1504, 1, 0x0014, 2809},
    {"IsInNewGamePlus", 1505, 1, 0x0014, 2810},
    {"IsInValhalla", 1506, 1, 0x0014, 2811},
    {"IsUltrawide", 1507, 1, 0x0014, 2812},
    {"IsSuperUltrawide", 1508, 1, 0x0014, 2813},
    {"UISettingUserReducedPuzzleHints", 1509, 1, 0x0014, 2814},
};

inline constexpr Field kFields_0525[] = {
    {"ExpressionTypes", 0, 12, 0x0024, 677},
};

inline constexpr Field kFields_0527[] = {
    {"Type", 0, 1, 0x0105, 4009},
};

inline constexpr Field kFields_0528[] = {
    {"Angle", 4, 4, 0x0008, 11015},
    {"Window", 8, 4, 0x0008, 11016},
    {"MinimumSpeed", 12, 4, 0x0008, 11017},
};

inline constexpr Field kFields_0529[] = {
    {"MoodOn", 1, 1, 0x0014, 2815},
    {"Mood", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_052A[] = {
    {"Angle", 4, 4, 0x0008, 11018},
    {"Window", 8, 4, 0x0008, 11019},
};

inline constexpr Field kFields_052C[] = {
    {"InVehicle", 1, 1, 0x0014, 2816},
};

inline constexpr Field kFields_052D[] = {
    {"uID", 0, 8, 0x0010, 0},
    {"WadName", 8, 8, 0x0018, 0},
    {"Position", 16, 0, 0x002C, 6},
    {"ForwardDir", 22, 0, 0x002C, 6},
    {"AdvanceRadiusMultiplier", 28, 4, 0x0008, 11026},
    {"OverrideFuzzRadius", 32, 4, 0x0008, 11027},
    {"InWorldMarkerDistance", 36, 4, 0x0008, 11028},
};

inline constexpr Field kFields_052E[] = {
    {"Coordinates", 0, 12, 0x0024, 678},
};

inline constexpr Field kFields_052F[] = {
    {"uID", 0, 8, 0x0010, 0},
    {"WadName", 8, 8, 0x0018, 0},
    {"Position", 16, 0, 0x002C, 6},
    {"Type", 24, 8, 0x0010, 0},
    {"AdvanceRadiusMultiplier", 32, 4, 0x0008, 11032},
    {"OverrideFuzzRadius", 36, 4, 0x0008, 11033},
};

inline constexpr Field kFields_0530[] = {
    {"Helpers", 0, 12, 0x0024, 679},
};

inline constexpr Field kFields_0531[] = {
    {"firstMarker", 0, 8, 0x0010, 0},
    {"secondMarker", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0532[] = {
    {"Edges", 0, 12, 0x0024, 680},
};

inline constexpr Field kFields_0533[] = {
    {"Flags", 0, 12, 0x0024, 681},
    {"LamsName", 12, 4, 0x0000, 1494},
    {"uID", 16, 8, 0x0010, 0},
    {"LamsDescription", 24, 4, 0x0000, 1495},
    {"InitState", 28, 1, 0x0104, 4015},
};

inline constexpr Field kFields_0534[] = {
    {"Icon", 32, 8, 0x0018, 0},
    {"Priority", 40, 4, 0x0000, 1498},
    {"FastTravel", 48, 8, 0x0010, 0},
    {"HeightOffset", 56, 4, 0x0008, 11034},
    {"Radius", 60, 4, 0x0008, 11035},
    {"OffsetX", 64, 4, 0x0008, 11036},
    {"OffsetY", 68, 4, 0x0008, 11037},
};

inline constexpr Field kFields_0535[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"LamsName", 8, 4, 0x0000, 1499},
    {"Discovery", 12, 1, 0x0104, 4017},
    {"Quests", 16, 12, 0x0024, 683},
};

inline constexpr Field kFields_0536[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"LamsName", 8, 4, 0x0000, 1500},
};

inline constexpr Field kFields_0537[] = {
    {"Markers", 32, 12, 0x0024, 685},
    {"PowerLevel", 44, 2, 0x0000, 1503},
    {"Wads", 48, 12, 0x0024, 686},
    {"Summary", 64, 12, 0x0024, 687},
};

inline constexpr Field kFields_0538[] = {
    {"Markers", 32, 12, 0x0024, 689},
    {"LamsResourceAvailable", 44, 4, 0x0000, 1506},
    {"Wads", 48, 12, 0x0024, 690},
    {"PowerLevel", 60, 2, 0x0000, 1507},
    {"Summary", 64, 12, 0x0024, 691},
    {"Areas", 80, 12, 0x0024, 692},
};

inline constexpr Field kFields_0539[] = {
    {"Regions", 32, 12, 0x0024, 694},
};

inline constexpr Field kFields_053A[] = {
    {"MapFlags", 0, 12, 0x0024, 695},
    {"Realms", 16, 12, 0x0024, 696},
    {"SummaryCategories", 32, 12, 0x0024, 697},
};

inline constexpr Field kFields_053B[] = {
    {"Type", 0, 1, 0x0105, 4021},
    {"ModStat", 8, 8, 0x001C, 231},
};

inline constexpr Field kFields_053C[] = {
    {"Fixed", 16, 4, 0x0000, 1510},
};

inline constexpr Field kFields_053D[] = {
    {"Min", 16, 4, 0x0000, 1511},
    {"Max", 20, 4, 0x0000, 1512},
};

inline constexpr Field kFields_053E[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"Amount", 8, 8, 0x001C, 1339},
    {"Type", 16, 1, 0x0104, 4024},
};

inline constexpr Field kFields_053F[] = {
    {"LootResults", 0, 12, 0x0024, 698},
    {"Weight", 12, 1, 0x0000, 1513},
    {"ModStat", 16, 8, 0x001C, 231},
};

inline constexpr Field kFields_0540[] = {
    {"Criterion", 0, 0, 0x002C, 256},
    {"Rolls", 24, 12, 0x0024, 699},
    {"Type", 36, 1, 0x0105, 4026},
    {"Conditions", 40, 12, 0x0024, 700},
};

inline constexpr Field kFields_0541[] = {
    {"Owner", 56, 1, 0x0104, 4029},
    {"MeterName", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_0542[] = {
    {"Owner", 56, 1, 0x0104, 4032},
    {"AttributeName", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_0543[] = {
    {"Pickup", 0, 8, 0x0010, 0},
    {"Stage", 8, 4, 0x0000, 1514},
};

inline constexpr Field kFields_0544[] = {
    {"Pickups", 56, 12, 0x0024, 707},
    {"Owner", 68, 1, 0x0104, 4035},
};

inline constexpr Field kFields_0546[] = {
    {"Wallet", 56, 8, 0x0010, 0},
    {"Recipe", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_0547[] = {
    {"Wallet", 56, 8, 0x0010, 0},
    {"Resource", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_0548[] = {
    {"QuestName", 56, 8, 0x0010, 0},
};

inline constexpr Field kFields_0549[] = {
    {"QuestName", 56, 8, 0x0010, 0},
    {"State", 64, 1, 0x0104, 4046},
};

inline constexpr Field kFields_054B[] = {
    {"EquipmentName", 56, 8, 0x0010, 0},
};

inline constexpr Field kFields_054C[] = {
    {"Wallet", 56, 8, 0x0010, 0},
    {"EquipmentName", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_054D[] = {
    {"EquipmentName", 56, 8, 0x0010, 0},
    {"EquipmentSlotSetName", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_054E[] = {
    {"AreaType", 56, 1, 0x0104, 4057},
};

inline constexpr Field kFields_0551[] = {
    {"WeaponType", 56, 4, 0x0000, 1515},
};

inline constexpr Field kFields_0552[] = {
    {"WeaponType", 56, 4, 0x0000, 1516},
};

inline constexpr Field kFields_0553[] = {
    {"CompanionID", 56, 8, 0x0010, 0},
};

inline constexpr Field kFields_0555[] = {
    {"Type", 0, 1, 0x0105, 4070},
};

inline constexpr Field kFields_0556[] = {
    {"True", 8, 8, 0x001C, 1365},
    {"False", 16, 8, 0x001C, 1365},
};

inline constexpr Field kFields_0557[] = {
    {"EntryType", 24, 1, 0x0104, 4073},
};

inline constexpr Field kFields_0558[] = {
    {"Name", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_0559[] = {
    {"Amount", 24, 4, 0x0000, 1517},
    {"Comparison", 28, 1, 0x0104, 4076},
};

inline constexpr Field kFields_055A[] = {
    {"Flag", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_055B[] = {
    {"PlayerName", 24, 8, 0x0010, 0},
};

inline constexpr Field kFields_055C[] = {
    {"CompanionID", 8, 0, 0x0028, 43},
    {"NotFound", 24, 8, 0x001C, 1365},
};

inline constexpr Field kFields_055E[] = {
    {"DistributorName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_055F[] = {
    {"MinRollSize", 4, 4, 0x0000, 1518},
    {"MaxRollSize", 8, 4, 0x0000, 1519},
    {"Next", 16, 8, 0x001C, 1365},
};

inline constexpr Field kFields_0560[] = {
    {"Wallet", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0561[] = {
    {"OverrideName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0562[] = {
    {"UseDropArc", 1, 1, 0x0014, 2817},
    {"ArcParamType", 2, 1, 0x0104, 4086},
    {"OverrideAutoInteractAvailability", 3, 1, 0x0014, 2818},
    {"CullDistance", 4, 4, 0x0008, 11058},
    {"ObjectName", 8, 8, 0x0010, 0},
    {"AcquireWallet", 16, 8, 0x0010, 791},
    {"TeardownWallet", 24, 8, 0x0010, 792},
    {"WakePhysicsJoint", 32, 8, 0x0010, 0},
    {"SpawnPositionJoint", 40, 8, 0x0010, 0},
    {"ArcTargetJoint", 48, 8, 0x0010, 0},
    {"ArcMinParam", 56, 4, 0x0008, 11059},
    {"ArcMaxParam", 60, 4, 0x0008, 11060},
    {"ArcTargetJointRadius", 64, 4, 0x0008, 11061},
    {"OverrideAlwaysUpright", 68, 1, 0x0014, 2819},
};

inline constexpr Field kFields_0563[] = {
    {"ConditionNames", 0, 0, 0x0028, 44},
};

inline constexpr Field kFields_0564[] = {
    {"ConditionMap", 0, 0, 0x0028, 45},
    {"ConditionSetMap", 16, 0, 0x0028, 46},
    {"DistributorMap", 32, 0, 0x0028, 47},
};

inline constexpr Field kFields_0565[] = {
    {"PrerequisiteFacts", 0, 12, 0x0024, 740},
    {"DescriptionLAMS", 12, 4, 0x0000, 1520},
};

inline constexpr Field kFields_0566[] = {
    {"PrerequisiteFacts", 0, 12, 0x0024, 741},
    {"DisplayNameLAMS", 12, 4, 0x0000, 1521},
    {"Weaknesses", 16, 12, 0x0024, 742},
    {"DescriptionLAMS", 28, 4, 0x0000, 1522},
    {"MaterialSwap", 32, 8, 0x0018, 0},
    {"LocationFoundLAMS", 40, 4, 0x0000, 1523},
    {"ElementLAMS", 44, 4, 0x0000, 1524},
};

inline constexpr Field kFields_0567[] = {
    {"DisplayNameLAMS", 0, 4, 0x0000, 1525},
    {"DescriptionLAMS", 4, 4, 0x0000, 1526},
    {"Members", 8, 12, 0x0024, 743},
};

inline constexpr Field kFields_0568[] = {
    {"BestiaryEntries", 0, 0, 0x0028, 48},
    {"Categories", 16, 12, 0x0024, 744},
};

inline constexpr Field kFields_0569[] = {
    {"DisplayNameLAMS", 0, 4, 0x0000, 1527},
    {"DescriptionLAMS", 4, 4, 0x0000, 1528},
    {"MaterialSwap", 8, 8, 0x0018, 0},
    {"PrerequisiteFacts", 16, 12, 0x0024, 745},
};

inline constexpr Field kFields_056A[] = {
    {"DisplayNameLAMS", 0, 4, 0x0000, 1529},
    {"DescriptionLAMS", 4, 4, 0x0000, 1530},
    {"Members", 8, 12, 0x0024, 746},
};

inline constexpr Field kFields_056B[] = {
    {"LoreEntries", 0, 0, 0x0028, 49},
    {"Categories", 16, 12, 0x0024, 747},
};

inline constexpr Field kFields_056C[] = {
    {"PrerequisiteFacts", 0, 12, 0x0024, 748},
    {"HeaderLAMS", 12, 4, 0x0000, 1531},
    {"TextLAMS", 16, 4, 0x0000, 1532},
    {"MaterialSwap", 24, 8, 0x0018, 0},
};

inline constexpr Field kFields_056D[] = {
    {"PrerequisiteFacts", 0, 12, 0x0024, 749},
    {"DisplayNameLAMS", 12, 4, 0x0000, 1533},
    {"Content", 16, 12, 0x0024, 750},
};

inline constexpr Field kFields_056E[] = {
    {"TopicNameLAMS", 0, 4, 0x0000, 1534},
    {"TopicHeaderLAMS", 4, 4, 0x0000, 1535},
    {"TopicIconMaterialSwap", 8, 8, 0x0018, 0},
    {"Entries", 16, 12, 0x0024, 751},
};

inline constexpr Field kFields_056F[] = {
    {"Topics", 0, 12, 0x0024, 752},
};

inline constexpr Field kFields_0570[] = {
    {"Pickup", 0, 2, 0x0000, 1536},
    {"Stage", 2, 1, 0x0000, 1537},
    {"Flags", 3, 1, 0x0204, 4087},
};

inline constexpr Field kFields_0571[] = {
    {"PerkThreshold", 4, 4, 0x0008, 11062},
    {"PerkTraitName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0572[] = {
    {"Attribute", 0, 8, 0x0010, 0},
    {"Mod", 8, 4, 0x0008, 11063},
    {"Flags", 12, 1, 0x0204, 4089},
};

inline constexpr Field kFields_0573[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"FlagsHasAny", 8, 12, 0x0024, 753},
    {"FlagsHasAll", 24, 12, 0x0024, 754},
    {"FlagsHasNone", 40, 12, 0x0024, 755},
};

inline constexpr Field kFields_0574[] = {
    {"Weight", 0, 2, 0x0000, 1540},
    {"Type", 2, 1, 0x0105, 4090},
};

inline constexpr Field kFields_0575[] = {
    {"RollData", 0, 12, 0x0024, 756},
};

inline constexpr Field kFields_0576[] = {
    {"RolledValue", 4, 0, 0x002C, 1392},
};

inline constexpr Field kFields_0577[] = {
    {"RolledValue", 8, 0, 0x002C, 1395},
};

inline constexpr Field kFields_0579[] = {
    {"RolledValue", 8, 0, 0x002C, 1563},
};

inline constexpr Field kFields_057A[] = {
    {"GOName", 0, 8, 0x0010, 0},
    {"UniqueName", 8, 8, 0x0010, 0},
    {"GOBehaviorTemplate", 16, 8, 0x001C, 470},
    {"ConfigName", 24, 8, 0x0010, 0},
    {"ObjectLevel", 32, 4, 0x0000, 1547},
    {"Heap", 36, 4, 0x0000, 1548},
};

inline constexpr Field kFields_057B[] = {
    {"DesignerFlags", 0, 12, 0x0024, 760},
    {"LamsName", 12, 4, 0x0000, 1549},
    {"Traits", 16, 12, 0x0024, 761},
    {"LamsDescription", 28, 4, 0x0000, 1550},
    {"Perks", 32, 12, 0x0024, 762},
    {"AltLamsDescription", 44, 4, 0x0000, 1551},
    {"EquipmentSlots", 48, 12, 0x0024, 763},
    {"CodeFlags", 60, 1, 0x0204, 4097},
    {"Joints", 64, 12, 0x0024, 764},
    {"EquipObjects", 80, 12, 0x0024, 765},
    {"Rolls", 96, 12, 0x0024, 766},
    {"UpgradeRecipes", 112, 12, 0x0024, 767},
    {"MiscRecipes", 128, 12, 0x0024, 768},
    {"Debug_CraftRecipes", 144, 12, 0x0024, 769},
    {"Name", 160, 8, 0x0010, 0},
    {"MemoryPool", 168, 8, 0x0010, 793},
    {"IconName", 176, 8, 0x0018, 0},
    {"SellRecipe", 184, 8, 0x0018, 0},
    {"BuybackRecipe", 192, 8, 0x0018, 0},
};

inline constexpr Field kFields_057C[] = {
    {"DefaultEquipmentName", 56, 8, 0x0010, 0},
    {"WalletName", 64, 8, 0x0010, 0},
};

inline constexpr Field kFields_057D[] = {
    {"EquipmentSlots", 0, 12, 0x0024, 773},
    {"NumLoadouts", 12, 4, 0x0000, 1552},
    {"AdditionalStatWallets", 16, 12, 0x0024, 774},
    {"Name", 32, 8, 0x0010, 0},
};

inline constexpr Field kFields_057E[] = {
    {"EquipmentMap", 0, 0, 0x0028, 50},
    {"Equipment", 16, 12, 0x0024, 775},
    {"EquipmentCharacterSlotSets", 32, 12, 0x0024, 776},
    {"EquipmentPools", 48, 0, 0x0028, 51},
    {"TelemetryTraits", 64, 12, 0x0024, 777},
    {"ExtraTransmogs", 80, 0, 0x0028, 52},
};

inline constexpr Field kFields_057F[] = {
    {"AttributeName", 0, 8, 0x0018, 0},
    {"LamsId", 8, 4, 0x0000, 1554},
    {"AttributeAmount", 12, 4, 0x0000, 1555},
};

inline constexpr Field kFields_0580[] = {
    {"Options", 0, 12, 0x0024, 778},
};

inline constexpr Field kFields_0581[] = {
    {"PrerequisiteFacts", 0, 12, 0x0024, 779},
    {"LamsId", 12, 4, 0x0000, 1556},
    {"AdditionalBonuses", 16, 12, 0x0024, 780},
    {"LamsDesc", 28, 4, 0x0000, 1557},
    {"TokenSlots", 32, 12, 0x0024, 781},
    {"AlternateLamsDesc", 44, 4, 0x0000, 1558},
    {"Data", 48, 0, 0x0028, 53},
    {"DefaultVisible", 60, 1, 0x0014, 2820},
    {"DefaultAcquired", 61, 1, 0x0014, 2821},
    {"Name", 64, 8, 0x0018, 0},
    {"Recipe", 72, 8, 0x0018, 0},
    {"PlatinumRecipe", 80, 8, 0x0018, 0},
    {"QuestCompletionGatingTokens", 88, 8, 0x0010, 0},
};

inline constexpr Field kFields_0582[] = {
    {"SkillTreeNodes", 0, 12, 0x0024, 782},
    {"LamsId", 12, 4, 0x0000, 1559},
    {"Data", 16, 0, 0x0028, 54},
    {"LamsDesc", 28, 4, 0x0000, 1560},
    {"Name", 32, 8, 0x0018, 0},
    {"TokenResource", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_0583[] = {
    {"TokenWallet", 0, 8, 0x0010, 794},
    {"SkillTrees", 8, 12, 0x0024, 783},
};

inline constexpr Field kFields_0585[] = {
    {"ArrowEmitterList", 0, 12, 0x0024, 784},
};

inline constexpr Field kFields_0586[] = {
    {"ConcussionList", 0, 12, 0x0024, 785},
};

inline constexpr Field kFields_0587[] = {
    {"UIEvent_MaxEventArgs", 0, 4, 0x0001, 1561},
    {"UIEvent_EventsPool", 4, 4, 0x0001, 1562},
    {"UIEvent_ListenersPool", 8, 4, 0x0001, 1563},
    {"UIEvent_ArgsPool", 12, 4, 0x0001, 1564},
    {"UIEvent_DelayedSendPool", 16, 4, 0x0001, 1565},
    {"UIState_StatesPool", 20, 4, 0x0001, 1566},
    {"UIState_EventListenersPool", 24, 4, 0x0001, 1567},
    {"UIState_TimersPool", 28, 4, 0x0001, 1568},
    {"UIState_ActivateStatesPool", 32, 4, 0x0001, 1569},
    {"UIState_PendingStatesPool", 36, 4, 0x0001, 1570},
    {"UITimer_UpdateListPool", 40, 4, 0x0001, 1571},
    {"UILayout_ItemsPool", 44, 4, 0x0001, 1572},
    {"UILayout_LayoutsPool", 48, 4, 0x0001, 1573},
    {"UILayout_SolverZone", 52, 4, 0x0001, 1574},
    {"UIList_ListsPool", 56, 4, 0x0001, 1575},
    {"UIList_UpdateListPool", 60, 4, 0x0001, 1576},
    {"UIList_ButtonsPool", 64, 4, 0x0001, 1577},
    {"UIList_ButtonGOListsPool", 68, 4, 0x0001, 1578},
    {"UIList_TableModelsPool", 72, 4, 0x0001, 1579},
    {"UICarousel_CarouselsPool", 76, 4, 0x0001, 1580},
    {"UICarousel_ItemListsPool", 80, 4, 0x0001, 1581},
    {"UICarousel_UpdateListPool", 84, 4, 0x0001, 1582},
    {"UIList2_ListsPool", 88, 4, 0x0001, 1583},
    {"UIList2_UpdateListPool", 92, 4, 0x0001, 1584},
    {"UIList2_ItemListsPool", 96, 4, 0x0001, 1585},
    {"UIList2_DataListsPool", 100, 4, 0x0001, 1586},
    {"UIList2_ModelsPool", 104, 4, 0x0001, 1587},
    {"UITextArea_TextAreasPool", 108, 4, 0x0001, 1588},
    {"UIClippingGroup_ItemsPool", 112, 4, 0x0001, 1589},
    {"UIClippingGroup_ClippingGroupsPool", 116, 4, 0x0001, 1590},
};

inline constexpr Field kFields_0588[] = {
    {"Styles", 0, 12, 0x0024, 786},
};

inline constexpr Field kFields_0589[] = {
    {"Unblockable", 0, 0, 0x002C, 0},
    {"Blockbreak", 8, 0, 0x002C, 0},
    {"Burstcounter", 16, 0, 0x002C, 0},
};

inline constexpr Field kFields_058A[] = {
    {"UnblockableGOName", 0, 8, 0x0010, 0},
    {"BlockbreakGOName", 8, 8, 0x0010, 0},
    {"BurstcounterGOName", 16, 8, 0x0010, 0},
    {"ColorSets", 24, 12, 0x0024, 787},
};

inline constexpr Field kFields_058B[] = {
    {"TargetType", 0, 8, 0x0018, 0},
    {"Priority", 8, 1, 0x0104, 4098},
};

inline constexpr Field kFields_058C[] = {
    {"TargetPriorities", 0, 12, 0x0024, 788},
};

inline constexpr Field kFields_058D[] = {
    {"DiffPerPriority", 0, 4, 0x0000, 1591},
    {"MaxLookAtDistance", 4, 4, 0x0000, 1592},
    {"IdleLookAtTimer", 8, 4, 0x0000, 1593},
    {"ReengageAngle", 12, 4, 0x0000, 1594},
    {"DisengageScreenPercent", 16, 4, 0x0008, 11078},
    {"MinTargetEngageBoxSize", 20, 4, 0x0008, 11079},
    {"ExtraMaxAngleForCamera", 24, 4, 0x0000, 1595},
    {"DetectionSquareShrinkSpeed", 28, 4, 0x0000, 1596},
    {"CombatTargetName", 32, 8, 0x0018, 795},
};

inline constexpr Field kFields_058E[] = {
    {"CategoryMap", 0, 0, 0x0028, 55},
};

inline constexpr Field kFields_058F[] = {
    {"BudgetMap", 0, 0, 0x0028, 56},
};

inline constexpr Field kFields_0590[] = {
    {"GPUPerformanceCategories", 0, 0, 0x002C, 1422},
    {"GPUPerformanceBudgets", 16, 0, 0x0028, 58},
    {"CPUPerformanceCategories", 32, 0, 0x002C, 1422},
    {"CPUPerformanceBudgets", 48, 0, 0x002C, 1423},
    {"GPUStatCategories", 64, 0, 0x002C, 1422},
    {"GPUStatBudgets", 80, 0, 0x002C, 1423},
    {"WwiseStatCategories", 96, 0, 0x002C, 1422},
    {"WwiseStatBudgets", 112, 0, 0x002C, 1423},
};

inline constexpr Field kFields_0591[] = {
    {"Position", 0, 4, 0x0008, 11084},
    {"FloatValue", 4, 4, 0x0008, 11085},
};

inline constexpr Field kFields_0592[] = {
    {"Start", 0, 4, 0x0008, 11086},
    {"End", 4, 4, 0x0008, 11087},
};

inline constexpr Field kFields_0593[] = {
    {"CyanRed", 0, 4, 0x0008, 11088},
    {"MagentaGreen", 4, 4, 0x0008, 11089},
    {"YellowBlue", 8, 4, 0x0008, 11090},
};

inline constexpr Field kFields_0594[] = {
    {"TemplateSymbol", 0, 8, 0x001A, 0},
    {"Pitch", 8, 4, 0x0008, 11091},
    {"Yaw", 12, 4, 0x0008, 11092},
    {"Length", 16, 4, 0x0008, 11093},
    {"AngleOfView", 32, 4, 0x0008, 11097},
    {"Tilt", 36, 4, 0x0008, 11098},
    {"TiltUp", 40, 4, 0x0008, 11099},
    {"TiltDown", 44, 4, 0x0008, 11100},
    {"Distance", 48, 4, 0x0008, 11101},
    {"Weight", 52, 4, 0x0008, 11102},
    {"Pitch_IsNull", 56, 1, 0x0016, 2822},
    {"Yaw_IsNull", 57, 1, 0x0016, 2823},
    {"Length_IsNull", 58, 1, 0x0016, 2824},
    {"X_IsNull", 59, 1, 0x0016, 2825},
    {"Y_IsNull", 60, 1, 0x0016, 2826},
    {"Z_IsNull", 61, 1, 0x0016, 2827},
    {"AngleOfView_IsNull", 62, 1, 0x0016, 2828},
    {"Tilt_IsNull", 63, 1, 0x0016, 2829},
    {"TiltUp_IsNull", 64, 1, 0x0016, 2830},
    {"TiltDown_IsNull", 65, 1, 0x0016, 2831},
    {"Distance_IsNull", 66, 1, 0x0016, 2832},
    {"Weight_IsNull", 67, 1, 0x0016, 2833},
};

inline constexpr Field kFields_0595[] = {
    {"FromCamera", 0, 8, 0x0018, 0},
    {"TweenTime", 8, 4, 0x0008, 11103},
    {"EaseIn", 12, 4, 0x0008, 11104},
    {"EaseOut", 16, 4, 0x0008, 11105},
    {"IgnoreIf", 20, 1, 0x0014, 2834},
    {"OnlyIf", 21, 1, 0x0014, 2835},
};

inline constexpr Field kFields_0596[] = {
    {"Type", 0, 4, 0x0000, 1597},
    {"Position", 4, 0, 0x002C, 6},
    {"ExitVector", 10, 0, 0x002C, 6},
    {"Radius", 16, 4, 0x0008, 11112},
    {"Height", 20, 4, 0x0008, 11113},
};

inline constexpr Field kFields_0597[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"SourceCode", 8, 8, 0x0018, 0},
    {"ByteCode", 16, 12, 0x0024, 789},
};

inline constexpr Field kFields_0598[] = {
    {"Start", 0, 0, 0x002C, 6},
    {"InTangent", 6, 0, 0x002C, 6},
    {"OutTangent", 12, 0, 0x002C, 6},
    {"End", 18, 0, 0x002C, 6},
};

inline constexpr Field kFields_0599[] = {
    {"PeakSlipRatio", 0, 4, 0x0008, 11126},
    {"PeakSlipAngle", 4, 4, 0x0008, 11127},
    {"LongitudinalGripAtPeak", 8, 4, 0x0008, 11128},
    {"LateralGripAtPeak", 12, 4, 0x0008, 11129},
    {"LongitudinalGripAtMaxSlip", 16, 4, 0x0008, 11130},
    {"LateralGripAtMaxSlip", 20, 4, 0x0008, 11131},
    {"Inertia", 24, 4, 0x0008, 11132},
};

inline constexpr Field kFields_059A[] = {
    {"Bump", 0, 4, 0x0008, 11133},
    {"Rebound", 4, 4, 0x0008, 11134},
};

inline constexpr Field kFields_059B[] = {
    {"Stiffness", 0, 4, 0x0008, 11135},
    {"DamperSpeedThreshold", 4, 4, 0x0008, 11136},
    {"DamperSlow", 8, 0, 0x002C, 1434},
    {"DamperFast", 16, 0, 0x002C, 1434},
};

inline constexpr Field kFields_059C[] = {
    {"PeakTorque", 0, 4, 0x0008, 11141},
    {"Inertia", 4, 4, 0x0008, 11142},
    {"IdleRPM", 8, 4, 0x0008, 11143},
    {"MaxRPM", 12, 4, 0x0008, 11144},
    {"EngineBrakePercentage", 16, 4, 0x0008, 11145},
};

inline constexpr Field kFields_059D[] = {
    {"TorqueDistribution", 0, 4, 0x0008, 11146},
    {"MinSpeedForLock", 4, 4, 0x0008, 11147},
    {"MaxSpeedForLock", 8, 4, 0x0008, 11148},
};

inline constexpr Field kFields_059E[] = {
    {"Ratio", 0, 4, 0x0008, 11149},
    {"ShiftDownPoint", 4, 4, 0x0008, 11150},
    {"ShiftUpPoint", 8, 4, 0x0008, 11151},
};

inline constexpr Field kFields_059F[] = {
    {"Gears", 0, 12, 0x0024, 790},
    {"TransmissionRatio", 12, 4, 0x0008, 11152},
};

inline constexpr Field kFields_05A0[] = {
    {"EnginePeakTorque", 0, 4, 0x0008, 11153},
    {"MaxSpeed", 4, 4, 0x0008, 11154},
    {"LongitudinalGripRecovery", 8, 4, 0x0008, 11155},
    {"LateralGripRecovery", 12, 4, 0x0008, 11156},
};

inline constexpr Field kFields_05A1[] = {
    {"SpeedForNoEffect", 0, 4, 0x0008, 11157},
    {"SpeedForFullEffect", 4, 4, 0x0008, 11158},
    {"MaxDeceleration", 8, 4, 0x0008, 11159},
};

inline constexpr Field kFields_05A2[] = {
    {"SteeringAtZeroSpeed", 0, 4, 0x0008, 11160},
    {"SteeringAtHighSpeed", 4, 4, 0x0008, 11161},
    {"HighSpeed", 8, 4, 0x0008, 11162},
};

inline constexpr Field kFields_05A3[] = {
    {"SpeedForNoEffect", 0, 4, 0x0008, 11163},
    {"SpeedForFullEffect", 4, 4, 0x0008, 11164},
    {"SteerMultiplierAtFullEffect", 8, 4, 0x0008, 11165},
};

inline constexpr Field kFields_05A4[] = {
    {"MinIntensity", 0, 4, 0x0008, 11166},
    {"MaxIntensity", 4, 4, 0x0008, 11167},
    {"MinAngle", 8, 4, 0x0008, 11168},
    {"MaxAngle", 12, 4, 0x0008, 11169},
    {"MaxSpeed", 16, 4, 0x0008, 11170},
};

inline constexpr Field kFields_05A5[] = {
    {"ActionList", 0, 12, 0x0024, 791},
};

inline constexpr Field kFields_05A7[] = {
    {"EnemyDynamicFlag", 0, 8, 0x0010, 0},
    {"Weight", 8, 4, 0x0008, 11173},
    {"FailFlags", 12, 2, 0x0204, 4100},
    {"CamRelativeMinAngleOverride", 16, 4, 0x0008, 11174},
    {"CamRelativeMaxAngleOverride", 20, 4, 0x0008, 11175},
};

inline constexpr Field kFields_05A8[] = {
    {"Status", 0, 1, 0x0104, 4101},
    {"JointName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_05A9[] = {
    {"PickupId", 0, 4, 0x0000, 1598},
    {"MinStage", 4, 2, 0x0000, 1599},
    {"MaxStage", 6, 2, 0x0000, 1600},
    {"Exclude", 8, 1, 0x0014, 2836},
};

inline constexpr Field kFields_05AA[] = {
    {"HitModifiers", 0, 12, 0x0024, 792},
    {"HitPoints", 12, 4, 0x0008, 11176},
};

inline constexpr Field kFields_05AB[] = {
    {"MinCountBlastedShrapnel", 0, 2, 0x0000, 1601},
    {"Concussion", 8, 8, 0x001C, 268},
};

inline constexpr Field kFields_05AC[] = {
    {"Width", 0, 4, 0x0008, 11177},
    {"Height", 4, 4, 0x0008, 11178},
    {"Direction", 8, 4, 0x0008, 11179},
    {"Position", 12, 4, 0x0008, 11180},
};

inline constexpr Field kFields_05AD[] = {
    {"Width", 0, 4, 0x0008, 11181},
    {"Height", 4, 4, 0x0008, 11182},
    {"Depth", 8, 4, 0x0008, 11183},
    {"Direction", 12, 4, 0x0008, 11184},
    {"Position", 16, 4, 0x0008, 11185},
};

inline constexpr Field kFields_05AE[] = {
    {"ModeConfigs", 0, 0, 0x0028, 65},
};

inline constexpr Field kFields_05AF[] = {
    {"WadName", 0, 8, 0x0010, 0},
    {"ZoneName", 8, 8, 0x0010, 0},
    {"Load", 16, 12, 0x0024, 793},
};

inline constexpr Field kFields_05B0[] = {
    {"WadName", 0, 8, 0x0010, 0},
    {"LogicGroupName", 8, 8, 0x0010, 0},
    {"ConfigurationName", 16, 8, 0x0010, 0},
    {"Load", 24, 12, 0x0024, 794},
};

inline constexpr Field kFields_05B1[] = {
    {"StrParams", 0, 12, 0x0024, 795},
    {"ConditionType", 12, 1, 0x0104, 4103},
    {"IntParams", 16, 12, 0x0024, 796},
    {"LogicGroupName", 32, 8, 0x0010, 0},
    {"ConfigurationName", 40, 8, 0x0010, 0},
};

inline constexpr Field kFields_05B2[] = {
    {"LoadGroupConfigs", 0, 12, 0x0024, 797},
    {"WadLZCount", 12, 4, 0x0000, 1603},
};

inline constexpr Field kFields_05B3[] = {
    {"HintDistance", 0, 4, 0x0008, 11186},
    {"ShowDistance", 4, 4, 0x0008, 11187},
    {"MaxFacingToObjectTolerance", 8, 4, 0x0008, 11188},
    {"MaxFacingToDirectionTolerance", 12, 4, 0x0008, 11189},
    {"MaxInFrontOfObjectAngleTolerance", 16, 4, 0x0008, 11190},
    {"PromptOffset", 20, 0, 0x002C, 6},
    {"PromptAtEndPosition", 26, 1, 0x0014, 2837},
    {"ShowPrompt", 27, 1, 0x0014, 2838},
};

inline constexpr Field kFields_05B4[] = {
    {"Time", 0, 4, 0x0008, 11194},
    {"Position", 4, 0, 0x002C, 6},
    {"OriginOffset", 10, 0, 0x002C, 6},
};

inline constexpr Field kFields_05B5[] = {
    {"GlobalBufferSize", 0, 4, 0x0000, 1604},
    {"PackBufferSize", 4, 4, 0x0000, 1605},
};

inline constexpr Field kFields_05B6[] = {
    {"StageList", 0, 12, 0x0024, 798},
};

inline constexpr Field kFields_05B7[] = {
    {"WadName", 0, 8, 0x0018, 0},
    {"CreatureName", 8, 8, 0x0018, 0},
    {"ConfigurationName", 16, 8, 0x0010, 0},
    {"CharacterName", 24, 4, 0x0000, 1606},
    {"AudioVariation", 28, 1, 0x0000, 1607},
};

inline constexpr Field kFields_05B8[] = {
    {"TrailName", 0, 8, 0x0010, 0},
    {"TrailModelName", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_05B9[] = {
    {"List", 0, 0, 0x0028, 66},
    {"PlayerTeams", 16, 12, 0x0024, 799},
};

inline constexpr Field kFields_05BA[] = {
    {"PickupId", 0, 2, 0x0000, 1608},
    {"Probability", 2, 1, 0x0004, 4106},
};

inline constexpr Field kFields_05BB[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Value", 8, 4, 0x0000, 1609},
};

inline constexpr Field kFields_05BC[] = {
    {"UnfreezeSpeed", 0, 4, 0x0008, 11201},
    {"UnfreezeDelay", 4, 4, 0x0008, 11202},
    {"FreezeMinTimeScale", 8, 4, 0x0008, 11203},
    {"FreezeMaxTimeScale", 12, 4, 0x0008, 11204},
};

inline constexpr Field kFields_05BD[] = {
    {"BaselineCameraDistance", 0, 4, 0x0008, 11205},
    {"NonScalingWorldOffset", 4, 4, 0x0008, 11206},
    {"MinimumScale", 8, 4, 0x0008, 11207},
    {"BaselinePlayerDistance", 12, 4, 0x0008, 11208},
    {"MinimumAlpha", 16, 4, 0x0008, 11209},
    {"AlphaBaselinePlayerDistance", 20, 4, 0x0008, 11210},
    {"CameraMinimumScale", 24, 4, 0x0008, 11211},
    {"BaselineCameraScaleDistance", 28, 4, 0x0008, 11212},
};

inline constexpr Field kFields_05BE[] = {
    {"IconName", 0, 8, 0x0010, 0},
    {"MeterName", 8, 8, 0x0010, 0},
    {"ExcludePlayer", 16, 1, 0x0014, 2839},
};

inline constexpr Field kFields_05BF[] = {
    {"Name", 0, 8, 0x0010, 0},
    {"CounterName", 8, 8, 0x0010, 0},
    {"Value", 16, 4, 0x0000, 1610},
    {"Trophy", 20, 1, 0x0000, 1611},
};

inline constexpr Field kFields_05C0[] = {
    {"Children", 0, 12, 0x0024, 800},
    {"Min", 12, 4, 0x0000, 1612},
    {"Threshold", 16, 12, 0x0024, 801},
    {"Max", 28, 4, 0x0000, 1613},
    {"Name", 32, 8, 0x0010, 0},
    {"CurrencyName", 40, 8, 0x0010, 0},
    {"Initial", 48, 4, 0x0000, 1614},
    {"CurrencyAdj", 52, 4, 0x0000, 1615},
};

inline constexpr Field kFields_05C1[] = {
    {"WadName", 0, 8, 0x0018, 0},
    {"CreatureName", 8, 8, 0x0018, 0},
    {"MarkerID", 16, 4, 0x0000, 1616},
};

inline constexpr Field kFields_05C2[] = {
    {"WadName", 0, 8, 0x0018, 0},
    {"CreatureName", 8, 8, 0x0018, 0},
    {"Config", 16, 8, 0x0010, 0},
    {"MarkerID", 24, 4, 0x0000, 1617},
    {"VisualScriptGlobalSpawnVariable", 32, 8, 0x0018, 0},
    {"CompanionBehaviorTreeName", 40, 8, 0x0018, 0},
    {"CompanionSubTreePath", 48, 8, 0x0018, 0},
    {"OtherBehaviorTreeName", 56, 8, 0x0018, 0},
    {"OtherSubTreePath", 64, 8, 0x0018, 0},
    {"AddWadName", 72, 8, 0x0018, 0},
};

inline constexpr Field kFields_05C3[] = {
    {"Probability", 0, 2, 0x0008, 11213},
    {"Min", 2, 1, 0x0000, 1618},
    {"Max", 3, 1, 0x0000, 1619},
};

inline constexpr Field kFields_05C4[] = {
    {"Loop", 0, 8, 0x0018, 0},
    {"RatchetForward", 8, 8, 0x0018, 0},
    {"RatchetBackward", 16, 8, 0x0018, 0},
    {"RatchetClickFCount", 24, 2, 0x0000, 1620},
    {"RatchetClickBCount", 26, 2, 0x0000, 1621},
};

inline constexpr Field kFields_05C5[] = {
    {"TargetJointList", 0, 12, 0x0024, 802},
    {"HeadTrackingJointRadius", 12, 4, 0x0008, 11214},
    {"TargetWeight", 16, 12, 0x0024, 803},
    {"ReticleScale", 28, 4, 0x0008, 11216},
    {"HeadTrackingJoint", 32, 8, 0x0018, 0},
    {"ReticleJoint", 40, 8, 0x0018, 0},
    {"PaintTargetReticle", 48, 8, 0x0018, 0},
    {"ReticleYOffset", 56, 4, 0x0008, 11217},
};

inline constexpr Field kFields_05C6[] = {
    {"WadName", 0, 8, 0x0018, 0},
    {"ActivityId", 8, 8, 0x0018, 0},
    {"MarkerName", 16, 8, 0x0010, 0},
    {"QuestName", 24, 8, 0x0010, 0},
    {"Type", 32, 1, 0x0104, 4107},
};

inline constexpr Field kFields_05C7[] = {
    {"Min", 0, 4, 0x0008, 11218},
    {"Max", 4, 4, 0x0008, 11219},
    {"AlwaysUse", 8, 1, 0x0014, 2840},
    {"Flags", 16, 8, 0x0204, 4108},
};

inline constexpr Field kFields_05C8[] = {
    {"LocationName", 0, 8, 0x0018, 0},
    {"VisualName", 8, 8, 0x0018, 0},
    {"StartTime", 16, 4, 0x0008, 11220},
    {"Points", 20, 4, 0x0008, 11221},
    {"FadeTime", 24, 4, 0x0008, 11222},
    {"PointFade", 28, 4, 0x0008, 11223},
};

inline constexpr Field kFields_05C9[] = {
    {"GOName", 0, 8, 0x0010, 0},
    {"FXName", 8, 8, 0x0010, 0},
    {"JointName", 16, 8, 0x0010, 0},
    {"PhysAttrOverride", 24, 8, 0x001C, 344},
};

inline constexpr Field kFields_05CA[] = {
    {"PlayFXList", 0, 0, 0x002C, 273},
    {"ShakingOutEffect", 16, 8, 0x0018, 0},
    {"BreakOutEffect", 24, 8, 0x0018, 0},
    {"ShakingOutSoundName", 32, 8, 0x0018, 796},
    {"BreakOutSoundName", 40, 8, 0x0018, 797},
    {"FreezingSoundName", 48, 8, 0x0018, 798},
    {"UnfreezingSoundName", 56, 8, 0x0018, 799},
    {"RotationWobble", 64, 8, 0x001C, 345},
    {"TranslationWobble", 72, 8, 0x001C, 345},
    {"MaxFrozenAmount", 80, 4, 0x0008, 11224},
    {"FreezingRate", 84, 4, 0x0008, 11225},
    {"FrozenTime", 88, 4, 0x0008, 11226},
    {"IgnoreDamageSeconds", 92, 4, 0x0008, 11227},
    {"ShatterImpactSpeed", 96, 4, 0x0008, 11228},
    {"Tags", 100, 4, 0x0204, 4109},
    {"FrozenHitPoints", 104, 4, 0x0008, 11229},
    {"ShaderSwapID", 108, 1, 0x0000, 1622},
};

inline constexpr Field kFields_05CB[] = {
    {"PieceList", 0, 12, 0x0024, 805},
    {"ShaderID", 12, 1, 0x0000, 1623},
    {"BasePhysicalAttributes", 16, 0, 0x002C, 344},
    {"OrbEmitter", 88, 8, 0x001C, 321},
    {"Bonus", 96, 8, 0x001C, 259},
    {"MaxShatterEffectTime", 104, 4, 0x0008, 11244},
    {"ShatterSoundName", 112, 8, 0x0018, 801},
    {"PlayFXList", 120, 0, 0x002C, 273},
};

inline constexpr Field kFields_05CC[] = {
    {"PlayFXList", 0, 0, 0x002C, 273},
    {"MainDecayFX", 16, 0, 0x002C, 272},
    {"AttachmentDecayFX", 144, 0, 0x002C, 272},
    {"AttachmentEmissionScale", 272, 4, 0x0008, 11273},
};

inline constexpr Field kFields_05CD[] = {
    {"Freezing", 0, 0, 0x002C, 1482},
    {"ShatterSystem", 112, 8, 0x001C, 1483},
};

inline constexpr Field kFields_05CE[] = {
    {"StandingMultiplier", 0, 4, 0x0008, 11280},
    {"MovingMultiplier", 4, 4, 0x0008, 11281},
    {"SprintingMultiplier", 8, 4, 0x0008, 11282},
    {"BoatingMultiplier", 12, 4, 0x0008, 11283},
    {"WolfSledMultiplier", 16, 4, 0x0008, 11284},
    {"BoatingMultiplierWithNavAssist", 20, 4, 0x0008, 11285},
    {"WolfSledMultiplierWithNavAssist", 24, 4, 0x0008, 11286},
};

inline constexpr Field kFields_05CF[] = {
    {"Realm", 0, 8, 0x0010, 0},
    {"Direction", 8, 0, 0x002C, 6},
};

inline constexpr Field kFields_05D0[] = {
    {"SetName", 0, 8, 0x0010, 0},
    {"CharacterType", 8, 1, 0x0104, 4118},
    {"AutomaticTriggerControl", 9, 1, 0x0104, 4119},
};

inline constexpr Field kFields_05D1[] = {
    {"InterruptLayers", 0, 12, 0x0024, 813},
};

inline constexpr Field kFields_05D2[] = {
    {"Low32Bits", 0, 4, 0x0004, 4120},
    {"High32Bits", 4, 4, 0x0004, 4121},
};

inline constexpr Field kFields_05D3[] = {
    {"ContentHash", 0, 0, 0x002C, 1490},
};

inline constexpr Field kFields_05D4[] = {
    {"ContentHash", 0, 0, 0x002C, 1490},
    {"MipCount", 8, 1, 0x0000, 1631},
};

inline constexpr Field kFields_05D5[] = {
    {"AnimName", 0, 8, 0x0010, 0},
    {"AnimBhvr", 8, 1, 0x0104, 4126},
    {"AnimTime", 12, 4, 0x0008, 11290},
    {"ResourceName", 16, 8, 0x0010, 0},
    {"MeterGranularity", 24, 4, 0x0008, 11291},
    {"Scale", 28, 4, 0x0008, 11292},
    {"AttributeOffset", 32, 4, 0x0008, 11293},
    {"MeterZeroWhenImmune", 36, 1, 0x0014, 2841},
    {"StatA", 40, 8, 0x001C, 231},
    {"StatB", 48, 8, 0x001C, 231},
    {"StatMin", 56, 4, 0x0008, 11294},
    {"StatMax", 60, 4, 0x0008, 11295},
    {"AnimTimeMin", 64, 4, 0x0008, 11296},
    {"AnimTimeMax", 68, 4, 0x0008, 11297},
};

inline constexpr Field kFields_05D6[] = {
    {"TextObjectName", 0, 8, 0x0018, 0},
    {"Driver", 8, 0, 0x002C, 1493},
    {"Values", 80, 12, 0x0024, 814},
};

inline constexpr Field kFields_05D7[] = {
    {"Anims", 0, 12, 0x0024, 815},
    {"StageNumber", 12, 4, 0x0000, 1632},
};

inline constexpr Field kFields_05D8[] = {
    {"PickupName", 0, 8, 0x0018, 0},
    {"Stages", 8, 12, 0x0024, 816},
    {"PickupLostAnims", 24, 12, 0x0024, 817},
};

inline constexpr Field kFields_05D9[] = {
    {"ReticleControlFlags", 0, 1, 0x0204, 4128},
    {"NonReticleScreenOffset", 4, 4, 0x0008, 11306},
    {"NonReticleMaxDistance", 8, 4, 0x0008, 11307},
};

inline constexpr Field kFields_05DA[] = {
    {"Arrow", 0, 8, 0x001C, 410},
    {"EmitJoint", 8, 8, 0x0018, 0},
    {"EmitFXList", 16, 8, 0x001C, 273},
    {"EmitOffset", 24, 0, 0x002C, 6},
    {"WeaponFlags", 30, 1, 0x0104, 4129},
    {"ConstraintLaunchAngleType", 31, 1, 0x0104, 4130},
    {"Scale", 32, 4, 0x0008, 11311},
    {"WeaponType", 36, 4, 0x0000, 1633},
    {"ConstraintAngle", 40, 4, 0x0008, 11312},
    {"ConstraintLaunchAngle", 44, 4, 0x0008, 11313},
    {"HorizontalAngleMin", 48, 4, 0x0008, 11314},
    {"HorizontalAngleMax", 52, 4, 0x0008, 11315},
    {"VerticalAngleMin", 56, 4, 0x0008, 11316},
    {"VerticalAngleMax", 60, 4, 0x0008, 11317},
    {"RandomizeLaunchAngleWithinConstraints", 64, 1, 0x0014, 2843},
};

inline constexpr Field kFields_05DB[] = {
    {"On", 0, 4, 0x0008, 11318},
    {"Concussion", 8, 8, 0x001C, 268},
};

inline constexpr Field kFields_05DC[] = {
    {"Behaviors", 0, 12, 0x0024, 818},
    {"QuestState", 12, 1, 0x0104, 4131},
};

inline constexpr Field kFields_05DD[] = {
    {"RayCastSourceJointName", 0, 8, 0x0010, 0},
    {"PositiveExtent", 8, 4, 0x0008, 11319},
    {"NegativeExtent", 12, 4, 0x0008, 11320},
};

inline constexpr Field kFields_05DE[] = {
    {"WeaponTrailType", 0, 8, 0x0010, 0},
    {"TrailName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_05DF[] = {
    {"Events", 0, 1, 0x0204, 4132},
    {"WeaponState", 1, 1, 0x0104, 4133},
    {"ThrowState", 2, 2, 0x0204, 4134},
    {"StuckState", 4, 1, 0x0204, 4135},
    {"Flags", 5, 1, 0x0204, 4136},
    {"MarkerList", 8, 12, 0x0024, 819},
    {"Priority", 20, 4, 0x0000, 1634},
    {"Move", 24, 8, 0x001C, 1283},
    {"FromMoveBlockList", 32, 12, 0x0024, 820},
};

inline constexpr Field kFields_05E0[] = {
    {"When", 0, 8, 0x0030, 65535},
    {"MaxClones", 8, 1, 0x0004, 4137},
};

inline constexpr Field kFields_05E1[] = {
    {"Time", 0, 4, 0x0008, 11321},
    {"Value", 4, 4, 0x0008, 11322},
};

inline constexpr Field kFields_05E2[] = {
    {"MPIconName", 0, 8, 0x0010, 0},
    {"MPIconHintName", 8, 8, 0x0010, 0},
    {"TraversalMove", 16, 8, 0x0010, 0},
    {"HintDistance", 24, 4, 0x0008, 11323},
    {"PromptOffset", 28, 0, 0x002C, 6},
    {"PromptAtEndPosition", 34, 1, 0x0014, 2844},
    {"MaxFacingToObjectTolerance", 36, 4, 0x0008, 11327},
    {"MaxFacingToDirectionTolerance", 40, 4, 0x0008, 11328},
    {"MaxInFrontOfObjectAngleTolerance", 44, 4, 0x0008, 11329},
};

inline constexpr Field kFields_05E3[] = {
    {"WeaponName", 0, 8, 0x0010, 0},
    {"ModeName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_05E4[] = {
    {"MinForce", 0, 4, 0x0008, 11330},
    {"MaxForce", 4, 4, 0x0008, 11331},
    {"MinSpeed", 8, 4, 0x0008, 11332},
    {"MaxSpeed", 12, 4, 0x0008, 11333},
    {"Type", 16, 1, 0x0104, 4138},
    {"Flags", 17, 1, 0x0204, 4139},
};

inline constexpr Field kFields_05E7[] = {
    {"Button", 0, 1, 0x0104, 4144},
    {"Force", 4, 4, 0x0008, 11342},
    {"Range", 8, 8, 0x001C, 1508},
};

inline constexpr Field kFields_05E8[] = {
    {"Chance", 0, 4, 0x0008, 11343},
    {"Value", 4, 4, 0x0008, 11344},
};

inline constexpr Field kFields_05E9[] = {
    {"Hash", 0, 12, 0x0024, 821},
};

inline constexpr Field kFields_05EA[] = {
    {"String", 0, 8, 0x0010, 0},
};

inline constexpr Field kFields_05EB[] = {
    {"ActiveStates", 0, 4, 0x0000, 1635},
    {"FXType", 4, 1, 0x0104, 4145},
    {"Name", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_05EC[] = {
    {"MainAxis", 0, 1, 0x0104, 4146},
    {"SecondaryAxis", 1, 1, 0x0104, 4147},
};

inline constexpr Field kFields_05ED[] = {
    {"Stage", 0, 1, 0x0000, 1636},
    {"Priority", 4, 4, 0x0000, 1637},
    {"MarkerCondition", 8, 8, 0x0010, 0},
    {"Concussions", 16, 12, 0x0024, 822},
};

inline constexpr Field kFields_05EE[] = {
    {"DetonateDelay", 0, 4, 0x0008, 11345},
    {"DetonateDelayScale", 4, 4, 0x0008, 11346},
    {"MinDetonateDelay", 8, 4, 0x0008, 11347},
    {"MaxDetonateDelay", 12, 4, 0x0008, 11348},
    {"BlendGroupFinalDelay", 16, 4, 0x0008, 11349},
};

inline constexpr Field kFields_05EF[] = {
    {"PercentTime", 0, 4, 0x0008, 11350},
    {"Offset", 4, 0, 0x002C, 6},
};

inline constexpr Field kFields_05F0[] = {
    {"PartnerMoveName", 0, 8, 0x0010, 0},
    {"MatchingMove", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_05F1[] = {
    {"On", 0, 4, 0x0008, 11354},
    {"Offset", 4, 0, 0x002C, 6},
    {"Concussion", 16, 8, 0x001C, 268},
};

inline constexpr Field kFields_05F2[] = {
    {"TimeOfEffect", 0, 2, 0x0008, 11358},
    {"AnimationWeight", 4, 4, 0x0008, 11359},
    {"SimulationStartJointName", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_05F3[] = {
    {"ParamName", 0, 8, 0x0010, 0},
    {"Value", 8, 4, 0x0008, 11360},
    {"ExpressionValue", 16, 8, 0x0030, 65535},
};

inline constexpr Field kFields_05F4[] = {
    {"ResourceName", 0, 8, 0x0010, 0},
    {"AltResourceName", 8, 8, 0x0010, 0},
    {"RequiredResource", 16, 8, 0x0010, 0},
    {"Wallet", 24, 8, 0x0010, 0},
    {"NGPOnly", 32, 1, 0x0014, 2845},
};

inline constexpr Field kFields_05F5[] = {
    {"EntitlementName", 0, 8, 0x0018, 0},
    {"ResourceInfos", 8, 12, 0x0024, 823},
};

inline constexpr Field kFields_05F6[] = {
    {"CombineOperator", 0, 1, 0x0104, 4148},
    {"Decision", 8, 8, 0x001C, 1043},
};

inline constexpr Field kFields_05F7[] = {
    {"BlockChance", 0, 4, 0x0008, 11361},
    {"HitFlags", 8, 8, 0x0204, 4149},
    {"BlockPartFlags", 16, 8, 0x0204, 4150},
};

inline constexpr Field kFields_05F8[] = {
    {"OnFailVsPlayer", 0, 1, 0x0104, 4151},
    {"OnFailVsFuture", 1, 1, 0x0104, 4152},
    {"OnFailVsFuturePlayer", 2, 1, 0x0104, 4153},
    {"OnFailVsNoCrossLine", 3, 1, 0x0104, 4154},
    {"OnFailVsPlayerCircle", 4, 1, 0x0104, 4155},
};

inline constexpr Field kFields_05F9[] = {
    {"NoCrossDistance", 0, 4, 0x0008, 11362},
    {"NoCrossTime", 4, 4, 0x0008, 11363},
    {"CircleRadius", 8, 4, 0x0008, 11364},
};

inline constexpr Field kFields_05FA[] = {
    {"Distance", 0, 4, 0x0008, 11365},
    {"Priority", 4, 4, 0x0008, 11366},
    {"InvalidMinTime", 8, 4, 0x0008, 11367},
    {"FutureDistance", 12, 4, 0x0008, 11368},
    {"FutureTime", 16, 4, 0x0008, 11369},
};

inline constexpr Field kFields_05FB[] = {
    {"Angle", 20, 4, 0x0008, 11375},
};

inline constexpr Field kFields_05FC[] = {
    {"Count", 20, 4, 0x0000, 1638},
};

inline constexpr Field kFields_05FD[] = {
    {"CenterAngle", 0, 4, 0x0008, 11381},
    {"SweepAngle", 4, 4, 0x0008, 11382},
    {"StartDistance", 8, 4, 0x0008, 11383},
    {"EndDistance", 12, 4, 0x0008, 11384},
    {"DistanceWeight", 16, 4, 0x0008, 11385},
    {"PriorityWeight", 20, 4, 0x0008, 11386},
};

inline constexpr Field kFields_05FE[] = {
    {"Type", 0, 1, 0x0104, 4156},
};

inline constexpr Field kFields_05FF[] = {
    {"Branch", 8, 8, 0x0010, 0},
    {"Refraction", 16, 4, 0x0008, 11387},
};

inline constexpr Field kFields_0600[] = {
    {"String", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0601[] = {
    {"PartitionID", 0, 12, 0x0024, 824},
    {"Data", 16, 12, 0x0024, 825},
};

inline constexpr Field kFields_0602[] = {
    {"CreatureJoint", 0, 8, 0x0010, 806},
    {"Joint", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0603[] = {
    {"Creature", 0, 8, 0x0010, 0},
    {"Joint", 8, 8, 0x0010, 0},
};

inline constexpr Field kFields_0604[] = {
    {"MoveName", 0, 8, 0x0010, 0},
    {"BlockChance", 8, 4, 0x0008, 11388},
    {"MinCnt", 12, 1, 0x0000, 1640},
    {"MaxCnt", 13, 1, 0x0000, 1641},
};

inline constexpr Field kFields_0605[] = {
    {"Context", 0, 8, 0x0010, 0},
    {"BlockChance", 8, 4, 0x0008, 11389},
    {"MinCnt", 12, 1, 0x0000, 1642},
    {"MaxCnt", 13, 1, 0x0000, 1643},
};

inline constexpr Field kFields_0606[] = {
    {"TrailJointName", 0, 8, 0x0010, 0},
};

inline constexpr Field kFields_0607[] = {
    {"GONameVariations", 0, 12, 0x0024, 826},
    {"ModeFlags", 12, 4, 0x0204, 4159},
    {"HideJoints", 16, 12, 0x0024, 827},
    {"Heap", 28, 4, 0x0000, 1644},
    {"WeaponRayCastList", 32, 12, 0x0024, 828},
    {"FadeTime", 44, 4, 0x0008, 11390},
    {"WeaponTrailJoints", 48, 12, 0x0024, 829},
    {"WeaponType", 60, 4, 0x0000, 1645},
    {"AttachModes", 64, 12, 0x0024, 830},
    {"Flags", 76, 2, 0x0204, 4160},
    {"ParentPickupId", 78, 2, 0x0000, 1646},
    {"GOName", 80, 8, 0x0018, 0},
    {"AttachName", 88, 8, 0x0010, 0},
    {"OutHandJoint", 96, 8, 0x0018, 0},
    {"SynchDefaultAnim", 104, 8, 0x0010, 0},
    {"HandEquippedAnim", 112, 8, 0x0010, 0},
    {"ChildAttachJoint", 120, 8, 0x0010, 0},
    {"CollisionFX", 128, 8, 0x001C, 272},
    {"SnapJointRemap", 136, 8, 0x001C, 1572},
    {"WeaponTrailJointData", 144, 8, 0x001C, 1137},
    {"InitialMode", 152, 8, 0x0010, 0},
    {"OnBackJoint", 160, 8, 0x0018, 0},
    {"WeaponName", 168, 8, 0x0010, 0},
    {"ReticleName", 176, 8, 0x0010, 0},
    {"Side", 184, 1, 0x0104, 4161},
    {"Type", 185, 1, 0x0105, 4162},
};

inline constexpr Field kFields_0608[] = {
    {"ThrowMode", 0, 8, 0x001C, 423},
    {"MinRotationSpeedMultiplier", 8, 4, 0x0008, 11391},
    {"MaxRotationSpeedMultiplier", 12, 4, 0x0008, 11392},
    {"MinSpeedMultiplier", 16, 4, 0x0008, 11393},
    {"MaxSpeedMultiplier", 20, 4, 0x0008, 11394},
    {"MinDeathTimePercent", 24, 4, 0x0008, 11395},
    {"MaxDeathTimePercent", 28, 4, 0x0008, 11396},
};

inline constexpr Field kFields_0609[] = {
    {"ReticleRadiusCheck", 192, 4, 0x0008, 11398},
    {"MaxThrowRange", 200, 0, 0x002C, 229},
    {"OnHitClass", 240, 8, 0x0018, 0},
};

inline constexpr Field kFields_060A[] = {
    {"MaxTargetLockOnDistance", 248, 0, 0x002C, 229},
    {"BoundarySurfaceNames", 288, 12, 0x0024, 844},
    {"CallbackDistance", 300, 4, 0x0008, 11419},
    {"ModeEquip", 304, 8, 0x0010, 807},
    {"ModeOutOfHand", 312, 8, 0x0010, 808},
    {"CatchHandJointName", 320, 8, 0x0010, 0},
    {"HitSurfaceJointName", 328, 8, 0x0010, 809},
    {"EmbedJointName", 336, 8, 0x0010, 810},
    {"ShoulderJointName", 344, 8, 0x0010, 811},
    {"StandardReturnMode", 352, 8, 0x001C, 423},
    {"DeathDropMode", 360, 8, 0x001C, 1544},
    {"DefaultBlock", 368, 8, 0x001C, 520},
    {"DebugHideClothJointName", 376, 8, 0x0010, 0},
    {"BoundaryConcussion", 384, 8, 0x001C, 268},
    {"TargetScaleMaxDistance", 392, 4, 0x0008, 11420},
    {"RespawnDelay", 396, 4, 0x0008, 11421},
    {"ScaleUpTime", 400, 4, 0x0008, 11422},
    {"BoundaryMaxFallDistance", 404, 4, 0x0008, 11423},
    {"BoundaryMaxDistanceFromOwner", 408, 4, 0x0008, 11424},
    {"BoundarySurfaceEntityTypes", 412, 4, 0x0204, 4180},
    {"EmbedThinShapeThreshold", 416, 4, 0x0008, 11425},
    {"MaxTimeThrownBeforeDetonationAllowed", 420, 4, 0x0008, 11426},
    {"EmbedPositionShrinkRatio", 424, 4, 0x0008, 11427},
    {"StatusPickup", 428, 2, 0x0000, 1653},
    {"SubType", 430, 1, 0x0105, 4181},
    {"IsAxe", 431, 1, 0x0014, 2846},
    {"AllowSkipNonPrecisionTargets", 432, 1, 0x0014, 2847},
    {"EmbedCreatureWorldPosition", 433, 1, 0x0014, 2848},
    {"UseCreatureTime", 434, 1, 0x0014, 2849},
    {"IsRegenWeapon", 435, 1, 0x0014, 2850},
    {"MaxNumberRegens", 436, 1, 0x0000, 1654},
    {"BoundaryResolution", 437, 1, 0x0104, 4182},
};

inline constexpr Field kFields_060B[] = {
    {"ModeName", 0, 8, 0x0010, 0},
    {"ChainDefaultLength", 8, 4, 0x0008, 11428},
    {"ConnectJointName", 16, 8, 0x0010, 0},
    {"ConnectJointOffset", 24, 0, 0x002C, 6},
    {"IsSheathed", 30, 1, 0x0014, 2851},
};

inline constexpr Field kFields_060C[] = {
    {"ChainHitProperties", 440, 0, 0x002C, 443},
    {"ChainProperties", 480, 0, 0x002C, 442},
    {"ChainAttachModes", 512, 12, 0x0024, 853},
    {"ChainLinkMinDistance", 524, 4, 0x0008, 11466},
    {"LinkObjectName", 528, 8, 0x0010, 817},
    {"LinkObjectShadowProxyName", 536, 8, 0x0010, 818},
    {"WeaponAttachJointName", 544, 8, 0x0010, 819},
    {"ElbowJointName", 552, 8, 0x0010, 820},
    {"ParentMotionRoot", 560, 8, 0x0010, 821},
    {"WristJointName", 568, 8, 0x0010, 822},
    {"HandJointName", 576, 8, 0x0010, 823},
    {"GrabJointName", 584, 8, 0x0010, 824},
    {"SoundChainJointName", 592, 8, 0x0010, 825},
    {"SoundBladeJointName", 600, 8, 0x0010, 826},
    {"ChainJointName", 608, 8, 0x0018, 0},
    {"CollisionRootName", 616, 8, 0x0018, 827},
    {"ChainLinkMaxDistance", 624, 4, 0x0008, 11467},
    {"ChainLinkScaleModel", 628, 4, 0x0008, 11468},
    {"DefaultSlackLengthIdle", 632, 4, 0x0008, 11469},
    {"DefaultSlackLengthStowed", 636, 4, 0x0008, 11470},
    {"SheatheSlackBlendDelay", 640, 4, 0x0008, 11471},
    {"SheatheSlackBlendTime", 644, 4, 0x0008, 11472},
    {"UnsheatheSlackBlendDelay", 648, 4, 0x0008, 11473},
    {"UnsheatheSlackBlendTime", 652, 4, 0x0008, 11474},
    {"NegativeRaycastExtent", 656, 4, 0x0008, 11475},
    {"PositiveRaycastExtent", 660, 4, 0x0008, 11476},
    {"ChainCollisionRadius", 664, 4, 0x0008, 11477},
    {"GravityFactor", 668, 4, 0x0008, 11478},
    {"AutoRecallLength", 672, 4, 0x0008, 11479},
    {"ChainDamping", 676, 4, 0x0008, 11480},
    {"ChainDampingY", 680, 4, 0x0008, 11481},
    {"ChainConstraint", 684, 4, 0x0008, 11482},
    {"BounceAmount", 688, 4, 0x0008, 11483},
    {"DeflectChainSpringForce", 692, 4, 0x0008, 11484},
    {"GrindHitPauseSpeed", 696, 4, 0x0008, 11485},
    {"GrindLinkExtraBladeSpeed", 700, 4, 0x0008, 11486},
    {"TargetingDistance", 704, 4, 0x0008, 11487},
    {"DefaultFlailLength", 708, 4, 0x0008, 11488},
    {"DisabledCollisionId", 712, 2, 0x0000, 1661},
    {"ChainCollisionId", 714, 2, 0x0000, 1662},
    {"MaxLinks", 716, 1, 0x0000, 1663},
    {"HeldLinkIndex", 717, 1, 0x0000, 1664},
    {"DeferredAttachJointUpdate", 718, 1, 0x0014, 2857},
    {"EnableFlailMode", 719, 1, 0x0014, 2858},
    {"RenderedAsRope", 720, 1, 0x0014, 2859},
};

inline constexpr Field kFields_060D[] = {
    {"LinearFriction", 0, 4, 0x0008, 11489},
    {"RollingFriction", 4, 4, 0x0008, 11490},
    {"Bounciness", 8, 4, 0x0008, 11491},
};

inline constexpr Field kFields_060E[] = {
    {"OverrideType", 0, 8, 0x0010, 0},
    {"RequestWeaponMode", 8, 8, 0x0010, 0},
    {"WeaponType", 16, 4, 0x0000, 1665},
    {"State", 20, 1, 0x0104, 4197},
    {"SwitchMode", 21, 1, 0x0104, 4198},
    {"AllowCacheInteract", 22, 1, 0x0014, 2860},
};

inline constexpr Field kFields_060F[] = {
    {"WeaponMode", 0, 8, 0x0010, 0},
    {"RequestStates", 8, 12, 0x0024, 854},
};

inline constexpr Field kFields_0610[] = {
    {"FromWeapon", 0, 4, 0x0000, 1666},
    {"SwitchMode", 4, 1, 0x0104, 4199},
    {"Interruptable", 5, 1, 0x0014, 2861},
    {"OverrideType", 8, 8, 0x0010, 0},
    {"Move", 16, 8, 0x001C, 1283},
};

inline constexpr Field kFields_0611[] = {
    {"ToWeapon", 0, 4, 0x0000, 1667},
    {"ToState", 4, 1, 0x0204, 4200},
    {"FromState", 5, 1, 0x0204, 4201},
    {"WeaponSwitchMoves", 8, 12, 0x0024, 855},
};

inline constexpr Field kFields_0612[] = {
    {"Angle", 0, 4, 0x0008, 11492},
    {"Radius", 4, 4, 0x0008, 11493},
};

inline constexpr Field kFields_0613[] = {
    {"Flags", 0, 1, 0x0204, 4202},
    {"ID", 1, 1, 0x0000, 1668},
};

inline constexpr Field kFields_0614[] = {
    {"GOName", 0, 8, 0x0010, 0},
    {"FXHide", 8, 8, 0x0010, 0},
    {"FXShow", 16, 8, 0x0010, 0},
    {"ContextFilter", 24, 4, 0x0000, 1669},
    {"Context", 28, 4, 0x0000, 1670},
    {"HitResistance", 32, 4, 0x0008, 11494},
    {"ReturnDelay", 36, 4, 0x0008, 11495},
    {"Size", 40, 4, 0x0008, 11496},
    {"Speed", 44, 4, 0x0008, 11497},
    {"Precision", 48, 4, 0x0008, 11498},
    {"Inertia", 52, 4, 0x0008, 11499},
    {"InertiaRetainHi", 56, 4, 0x0008, 11500},
    {"HoveringSpeed", 60, 4, 0x0008, 11501},
    {"HoveringSpeedRange", 64, 4, 0x0008, 11502},
    {"HoveringInertia", 68, 4, 0x0008, 11503},
    {"RoamingSpeed", 72, 4, 0x0008, 11504},
    {"RoamingHeight", 76, 4, 0x0008, 11505},
    {"RoamingHeightRange", 80, 4, 0x0008, 11506},
    {"Flags", 84, 1, 0x0204, 4203},
};

inline constexpr Field kFields_0615[] = {
    {"Type", 0, 1, 0x0104, 4204},
};

inline constexpr Field kFields_0616[] = {
    {"JointName", 8, 8, 0x0010, 0},
    {"Radius", 16, 4, 0x0008, 11507},
};

inline constexpr Field kFields_0617[] = {
    {"JointNames", 8, 12, 0x0024, 856},
};

inline constexpr Field kFields_0618[] = {
    {"ShotEmitter", 120, 8, 0x001C, 411},
    {"ShotMeterAmount", 128, 4, 0x0008, 11515},
    {"MolotovEmitter", 136, 8, 0x001C, 411},
    {"MolotovMeterAmount", 144, 4, 0x0008, 11516},
    {"MolotovChargeTime", 148, 4, 0x0008, 11517},
    {"FireHitContext", 152, 8, 0x0010, 0},
    {"FireHitFlags", 160, 8, 0x0204, 4210},
    {"FireDamage", 168, 4, 0x0008, 11518},
    {"FireDamageFreq", 172, 4, 0x0008, 11519},
};

inline constexpr Field kFields_0619[] = {
    {"OlympusBlast", 120, 8, 0x001C, 268},
    {"OlympusStrike", 128, 8, 0x001C, 268},
    {"OlympusTornado", 136, 8, 0x001C, 268},
    {"RainGO", 144, 8, 0x0018, 828},
    {"RainArrowCnc", 152, 8, 0x001C, 268},
    {"RainArrowCsh", 160, 8, 0x001C, 997},
    {"RainArrowFfb", 168, 8, 0x001C, 274},
    {"OlympusBlastMeterAmount", 176, 4, 0x0008, 11527},
    {"OlympusStrikeMeterAmount", 180, 4, 0x0008, 11528},
    {"OlympusTornadoMeterAmount", 184, 4, 0x0008, 11529},
    {"MashometerDrainRate", 188, 4, 0x0008, 11530},
    {"MashometerMax", 192, 4, 0x0008, 11531},
    {"RainMinAnimSpeed", 196, 4, 0x0008, 11532},
    {"RainMaxAnimSpeed", 200, 4, 0x0008, 11533},
    {"RainMaxTime", 204, 4, 0x0008, 11534},
    {"RainNoMashMaxTime", 208, 4, 0x0008, 11535},
    {"RainMaxRadius", 212, 4, 0x0008, 11536},
    {"RainMinRadius", 216, 4, 0x0008, 11537},
    {"RainArrowMaxSpawnTime", 220, 4, 0x0008, 11538},
    {"RainArrowMinSpawnTime", 224, 4, 0x0008, 11539},
    {"RainArrowMinRotVel", 228, 4, 0x0008, 11540},
    {"RainArrowMaxRotVel", 232, 4, 0x0008, 11541},
    {"RainArrowGravity", 236, 4, 0x0008, 11542},
    {"RainArrowInitialScale", 240, 4, 0x0008, 11543},
    {"RainArrowMinScale", 244, 4, 0x0008, 11544},
    {"RainArrowMaxScale", 248, 4, 0x0008, 11545},
    {"RainArrowScaleTime", 252, 4, 0x0008, 11546},
    {"RainMaxArrows", 256, 1, 0x0000, 1675},
};

inline constexpr Field kFields_061A[] = {
    {"ExpressionType", 0, 2, 0x0104, 4214},
    {"ReturnType", 2, 1, 0x0104, 4215},
    {"ReferenceTermType", 3, 1, 0x0104, 4216},
};

inline constexpr Field kFields_061B[] = {
    {"Attribute", 0, 8, 0x0010, 0},
    {"Min", 8, 4, 0x0008, 11547},
    {"Max", 12, 4, 0x0008, 11548},
    {"Flags", 16, 1, 0x0204, 4217},
};

inline constexpr Field kFields_061C[] = {
    {"Size", 0, 4, 0x0000, 1676},
    {"Type", 4, 1, 0x0104, 4218},
};

inline constexpr Field kFields_061D[] = {
    {"EquipmentNames", 0, 12, 0x0024, 867},
};

inline constexpr Field kFields_061E[] = {
    {"Name", 0, 8, 0x0018, 0},
    {"Color", 16, 0, 0x002C, 1},
};

inline constexpr Field kFields_061F[] = {
    {"DifficultyName", 0, 8, 0x0010, 0},
    {"ModifierSet", 8, 8, 0x001C, 286},
    {"MagicCostMult", 16, 4, 0x0008, 11553},
    {"WiggleMeterDrainMult", 20, 4, 0x0008, 11554},
    {"ProximityWeightLimitMult", 24, 4, 0x0008, 11555},
    {"FightWeightLimitMult", 28, 4, 0x0008, 11556},
    {"ProjectileWeightLimitMult", 32, 4, 0x0008, 11557},
    {"RecoveryTimeMult", 36, 4, 0x0008, 11558},
    {"ForceActionTimeMult", 40, 4, 0x0008, 11559},
};

inline constexpr Field kFields_0620[] = {
    {"List", 0, 12, 0x0024, 868},
    {"TeamPickups", 16, 12, 0x0024, 869},
};

inline constexpr Field kFields_0621[] = {
    {"Text", 0, 8, 0x0018, 0},
    {"LamsId", 8, 4, 0x0000, 1678},
};

inline constexpr Field kFields_0622[] = {
    {"TrailJointName", 0, 8, 0x0010, 0},
    {"TrailJointOffset", 8, 0, 0x002C, 6},
};

inline constexpr Field kFields_0623[] = {
    {"RayCastSourceJointName", 0, 8, 0x0010, 0},
    {"PositiveExtent", 8, 4, 0x0008, 11563},
    {"NegativeExtent", 12, 4, 0x0008, 11564},
};

inline constexpr Field kFields_0624[] = {
    {"fromJoint", 0, 8, 0x0010, 0},
    {"toJoint", 8, 8, 0x0018, 0},
};

inline constexpr Field kFields_0625[] = {
    {"SheathTable", 0, 0, 0x0028, 67},
    {"ModeFlags", 12, 4, 0x0204, 4220},
    {"SheathRestoreTable", 16, 0, 0x0028, 68},
    {"Scale", 28, 4, 0x0008, 11565},
    {"ModeName", 32, 8, 0x0010, 0},
    {"JointName", 40, 8, 0x0010, 0},
    {"TargetGroup", 48, 8, 0x0010, 0},
    {"OffsetX", 56, 4, 0x0008, 11566},
    {"OffsetY", 60, 4, 0x0008, 11567},
    {"OffsetZ", 64, 4, 0x0008, 11568},
    {"AttachModeType", 68, 1, 0x0104, 4221},
};

inline constexpr Struct kStructs[] = {
    {0x0000, 125, kFields_0000},
    {0x0001, 1, kFields_0001},
    {0x000B, 8, kFields_000B},
    {0x000C, 6, kFields_000C},
    {0x000D, 2, kFields_000D},
    {0x000E, 7, kFields_000E},
    {0x000F, 11, kFields_000F},
    {0x0010, 247, kFields_0010},
    {0x0011, 58, kFields_0011},
    {0x0012, 3, kFields_0012},
    {0x0013, 168, kFields_0013},
    {0x0014, 163, kFields_0014},
    {0x0015, 19, kFields_0015},
    {0x0016, 2, kFields_0016},
    {0x0017, 5, kFields_0017},
    {0x0018, 13, kFields_0018},
    {0x0019, 24, kFields_0019},
    {0x001A, 3, kFields_001A},
    {0x001B, 3, kFields_001B},
    {0x001C, 3, kFields_001C},
    {0x001D, 3, kFields_001D},
    {0x001E, 10, kFields_001E},
    {0x001F, 1, kFields_001F},
    {0x0020, 2, kFields_0020},
    {0x0021, 58, kFields_0021},
    {0x0022, 3, kFields_0022},
    {0x0023, 8, kFields_0023},
    {0x0024, 2, kFields_0024},
    {0x0025, 2, kFields_0025},
    {0x0026, 1, kFields_0026},
    {0x0027, 6, kFields_0027},
    {0x0028, 2, kFields_0028},
    {0x0029, 4, kFields_0029},
    {0x002A, 1, kFields_002A},
    {0x002B, 2, kFields_002B},
    {0x002C, 3, kFields_002C},
    {0x002D, 6, kFields_002D},
    {0x002E, 10, kFields_002E},
    {0x002F, 33, kFields_002F},
    {0x0030, 9, kFields_0030},
    {0x0031, 27, kFields_0031},
    {0x0032, 21, kFields_0032},
    {0x0033, 9, kFields_0033},
    {0x0034, 3, kFields_0034},
    {0x0035, 3, kFields_0035},
    {0x0036, 9, kFields_0036},
    {0x0037, 31, kFields_0037},
    {0x0038, 99, kFields_0038},
    {0x0039, 5, kFields_0039},
    {0x003A, 26, kFields_003A},
    {0x003B, 17, kFields_003B},
    {0x003C, 59, kFields_003C},
    {0x003D, 83, kFields_003D},
    {0x003E, 27, kFields_003E},
    {0x003F, 35, kFields_003F},
    {0x0040, 12, kFields_0040},
    {0x0041, 7, kFields_0041},
    {0x0042, 3, kFields_0042},
    {0x0043, 3, kFields_0043},
    {0x0044, 7, kFields_0044},
    {0x0045, 7, kFields_0045},
    {0x0046, 18, kFields_0046},
    {0x0047, 2, kFields_0047},
    {0x0048, 2, kFields_0048},
    {0x0049, 1, kFields_0049},
    {0x004A, 4, kFields_004A},
    {0x004B, 1, kFields_004B},
    {0x004C, 2, kFields_004C},
    {0x004D, 46, kFields_004D},
    {0x004E, 3, kFields_004E},
    {0x004F, 35, kFields_004F},
    {0x0050, 17, kFields_0050},
    {0x0051, 8, kFields_0051},
    {0x0052, 9, kFields_0052},
    {0x0053, 6, kFields_0053},
    {0x0055, 4, kFields_0055},
    {0x0056, 8, kFields_0056},
    {0x0057, 2, kFields_0057},
    {0x0058, 6, kFields_0058},
    {0x0059, 7, kFields_0059},
    {0x005A, 297, kFields_005A},
    {0x005B, 1, kFields_005B},
    {0x005C, 7, kFields_005C},
    {0x005D, 5, kFields_005D},
    {0x005E, 4, kFields_005E},
    {0x005F, 11, kFields_005F},
    {0x0060, 1, kFields_0060},
    {0x0061, 4, kFields_0061},
    {0x0063, 1, kFields_0063},
    {0x0064, 2, kFields_0064},
    {0x0065, 4, kFields_0065},
    {0x0066, 3, kFields_0066},
    {0x0067, 11, kFields_0067},
    {0x0068, 20, kFields_0068},
    {0x0069, 3, kFields_0069},
    {0x006A, 8, kFields_006A},
    {0x006B, 10, kFields_006B},
    {0x006C, 15, kFields_006C},
    {0x006D, 1, kFields_006D},
    {0x006E, 3, kFields_006E},
    {0x006F, 2, kFields_006F},
    {0x0070, 2, kFields_0070},
    {0x0071, 2, kFields_0071},
    {0x0072, 7, kFields_0072},
    {0x0073, 7, kFields_0073},
    {0x0074, 7, kFields_0074},
    {0x0075, 4, kFields_0075},
    {0x0076, 6, kFields_0076},
    {0x0077, 5, kFields_0077},
    {0x0078, 7, kFields_0078},
    {0x007A, 4, kFields_007A},
    {0x007B, 59, kFields_007B},
    {0x007C, 4, kFields_007C},
    {0x007D, 1, kFields_007D},
    {0x007E, 6, kFields_007E},
    {0x007F, 1, kFields_007F},
    {0x0080, 20, kFields_0080},
    {0x0081, 175, kFields_0081},
    {0x0082, 7, kFields_0082},
    {0x0083, 41, kFields_0083},
    {0x0084, 2, kFields_0084},
    {0x0085, 1, kFields_0085},
    {0x0086, 54, kFields_0086},
    {0x0087, 1, kFields_0087},
    {0x0088, 1, kFields_0088},
    {0x0089, 1, kFields_0089},
    {0x008A, 1, kFields_008A},
    {0x008B, 1, kFields_008B},
    {0x008C, 7, kFields_008C},
    {0x008D, 1, kFields_008D},
    {0x008E, 7, kFields_008E},
    {0x008F, 5, kFields_008F},
    {0x0090, 21, kFields_0090},
    {0x0091, 4, kFields_0091},
    {0x0092, 7, kFields_0092},
    {0x0093, 5, kFields_0093},
    {0x0094, 2, kFields_0094},
    {0x0095, 9, kFields_0095},
    {0x0096, 5, kFields_0096},
    {0x0098, 2, kFields_0098},
    {0x0099, 2, kFields_0099},
    {0x009A, 2, kFields_009A},
    {0x009B, 2, kFields_009B},
    {0x009C, 1, kFields_009C},
    {0x009D, 1, kFields_009D},
    {0x00A1, 16, kFields_00A1},
    {0x00A2, 4, kFields_00A2},
    {0x00A3, 3, kFields_00A3},
    {0x00A4, 10, kFields_00A4},
    {0x00A5, 6, kFields_00A5},
    {0x00A6, 5, kFields_00A6},
    {0x00A7, 21, kFields_00A7},
    {0x00A8, 8, kFields_00A8},
    {0x00A9, 2, kFields_00A9},
    {0x00AA, 6, kFields_00AA},
    {0x00AB, 3, kFields_00AB},
    {0x00AC, 3, kFields_00AC},
    {0x00AD, 3, kFields_00AD},
    {0x00AE, 8, kFields_00AE},
    {0x00AF, 2, kFields_00AF},
    {0x00B0, 3, kFields_00B0},
    {0x00B1, 13, kFields_00B1},
    {0x00B2, 6, kFields_00B2},
    {0x00B3, 3, kFields_00B3},
    {0x00B4, 2, kFields_00B4},
    {0x00B5, 1, kFields_00B5},
    {0x00B6, 3, kFields_00B6},
    {0x00B7, 7, kFields_00B7},
    {0x00B8, 15, kFields_00B8},
    {0x00B9, 21, kFields_00B9},
    {0x00BA, 7, kFields_00BA},
    {0x00BB, 9, kFields_00BB},
    {0x00BC, 21, kFields_00BC},
    {0x00BD, 2, kFields_00BD},
    {0x00BE, 3, kFields_00BE},
    {0x00BF, 2, kFields_00BF},
    {0x00C0, 1, kFields_00C0},
    {0x00C1, 6, kFields_00C1},
    {0x00C2, 1, kFields_00C2},
    {0x00C3, 3, kFields_00C3},
    {0x00C4, 1, kFields_00C4},
    {0x00C5, 2, kFields_00C5},
    {0x00C6, 3, kFields_00C6},
    {0x00C7, 1, kFields_00C7},
    {0x00C8, 1, kFields_00C8},
    {0x00C9, 1, kFields_00C9},
    {0x00CA, 10, kFields_00CA},
    {0x00CB, 2, kFields_00CB},
    {0x00CC, 2, kFields_00CC},
    {0x00CD, 3, kFields_00CD},
    {0x00CE, 3, kFields_00CE},
    {0x00CF, 1, kFields_00CF},
    {0x00D1, 4, kFields_00D1},
    {0x00D2, 4, kFields_00D2},
    {0x00D3, 7, kFields_00D3},
    {0x00D4, 11, kFields_00D4},
    {0x00D5, 6, kFields_00D5},
    {0x00D6, 15, kFields_00D6},
    {0x00D7, 3, kFields_00D7},
    {0x00D8, 5, kFields_00D8},
    {0x00D9, 10, kFields_00D9},
    {0x00DA, 3, kFields_00DA},
    {0x00DB, 1, kFields_00DB},
    {0x00DC, 1, kFields_00DC},
    {0x00DD, 5, kFields_00DD},
    {0x00DE, 3, kFields_00DE},
    {0x00DF, 2, kFields_00DF},
    {0x00E0, 1, kFields_00E0},
    {0x00E1, 3, kFields_00E1},
    {0x00E2, 2, kFields_00E2},
    {0x00E3, 2, kFields_00E3},
    {0x00E4, 5, kFields_00E4},
    {0x00E5, 10, kFields_00E5},
    {0x00E6, 1, kFields_00E6},
    {0x00E7, 12, kFields_00E7},
    {0x00E8, 3, kFields_00E8},
    {0x00E9, 3, kFields_00E9},
    {0x00EA, 3, kFields_00EA},
    {0x00EB, 3, kFields_00EB},
    {0x00ED, 10, kFields_00ED},
    {0x00EE, 2, kFields_00EE},
    {0x00EF, 28, kFields_00EF},
    {0x00F0, 6, kFields_00F0},
    {0x00F1, 2, kFields_00F1},
    {0x00F2, 2, kFields_00F2},
    {0x00F3, 2, kFields_00F3},
    {0x00F4, 4, kFields_00F4},
    {0x00F5, 3, kFields_00F5},
    {0x00F6, 1, kFields_00F6},
    {0x00F7, 11, kFields_00F7},
    {0x00F8, 2, kFields_00F8},
    {0x00F9, 2, kFields_00F9},
    {0x00FA, 2, kFields_00FA},
    {0x00FB, 1, kFields_00FB},
    {0x00FC, 1, kFields_00FC},
    {0x00FD, 3, kFields_00FD},
    {0x00FE, 1, kFields_00FE},
    {0x00FF, 8, kFields_00FF},
    {0x0100, 4, kFields_0100},
    {0x0101, 8, kFields_0101},
    {0x0102, 5, kFields_0102},
    {0x0103, 3, kFields_0103},
    {0x0104, 2, kFields_0104},
    {0x0105, 7, kFields_0105},
    {0x0106, 18, kFields_0106},
    {0x0107, 19, kFields_0107},
    {0x0108, 2, kFields_0108},
    {0x0109, 10, kFields_0109},
    {0x010A, 6, kFields_010A},
    {0x010B, 6, kFields_010B},
    {0x010C, 26, kFields_010C},
    {0x010D, 4, kFields_010D},
    {0x010E, 1, kFields_010E},
    {0x010F, 1, kFields_010F},
    {0x0110, 23, kFields_0110},
    {0x0111, 1, kFields_0111},
    {0x0112, 18, kFields_0112},
    {0x0113, 1, kFields_0113},
    {0x0114, 6, kFields_0114},
    {0x0115, 7, kFields_0115},
    {0x0116, 4, kFields_0116},
    {0x0117, 56, kFields_0117},
    {0x0118, 1, kFields_0118},
    {0x0119, 1, kFields_0119},
    {0x011A, 1, kFields_011A},
    {0x011B, 4, kFields_011B},
    {0x011C, 4, kFields_011C},
    {0x011D, 3, kFields_011D},
    {0x011E, 6, kFields_011E},
    {0x011F, 4, kFields_011F},
    {0x0120, 14, kFields_0120},
    {0x0121, 3, kFields_0121},
    {0x0122, 15, kFields_0122},
    {0x0123, 4, kFields_0123},
    {0x0124, 5, kFields_0124},
    {0x0125, 16, kFields_0125},
    {0x0126, 3, kFields_0126},
    {0x0127, 2, kFields_0127},
    {0x0128, 3, kFields_0128},
    {0x0129, 19, kFields_0129},
    {0x012A, 5, kFields_012A},
    {0x012B, 3, kFields_012B},
    {0x012C, 2, kFields_012C},
    {0x012D, 12, kFields_012D},
    {0x012E, 3, kFields_012E},
    {0x012F, 2, kFields_012F},
    {0x0130, 24, kFields_0130},
    {0x0131, 11, kFields_0131},
    {0x0132, 2, kFields_0132},
    {0x0133, 65, kFields_0133},
    {0x0134, 2, kFields_0134},
    {0x0135, 2, kFields_0135},
    {0x0136, 2, kFields_0136},
    {0x0137, 2, kFields_0137},
    {0x0138, 2, kFields_0138},
    {0x0139, 3, kFields_0139},
    {0x013A, 8, kFields_013A},
    {0x013B, 1, kFields_013B},
    {0x013C, 1, kFields_013C},
    {0x013D, 1, kFields_013D},
    {0x013E, 125, kFields_013E},
    {0x013F, 1, kFields_013F},
    {0x0140, 14, kFields_0140},
    {0x0141, 13, kFields_0141},
    {0x0142, 16, kFields_0142},
    {0x0143, 5, kFields_0143},
    {0x0144, 9, kFields_0144},
    {0x0145, 3, kFields_0145},
    {0x0146, 5, kFields_0146},
    {0x0147, 4, kFields_0147},
    {0x0148, 1, kFields_0148},
    {0x0149, 9, kFields_0149},
    {0x014A, 3, kFields_014A},
    {0x014B, 6, kFields_014B},
    {0x014C, 2, kFields_014C},
    {0x014D, 2, kFields_014D},
    {0x014E, 3, kFields_014E},
    {0x014F, 12, kFields_014F},
    {0x0150, 3, kFields_0150},
    {0x0151, 2, kFields_0151},
    {0x0152, 6, kFields_0152},
    {0x0153, 16, kFields_0153},
    {0x0154, 9, kFields_0154},
    {0x0155, 10, kFields_0155},
    {0x0156, 3, kFields_0156},
    {0x0157, 13, kFields_0157},
    {0x0158, 18, kFields_0158},
    {0x0159, 4, kFields_0159},
    {0x015A, 6, kFields_015A},
    {0x015B, 3, kFields_015B},
    {0x015C, 2, kFields_015C},
    {0x015D, 4, kFields_015D},
    {0x015E, 2, kFields_015E},
    {0x015F, 3, kFields_015F},
    {0x0160, 2, kFields_0160},
    {0x0161, 3, kFields_0161},
    {0x0162, 4, kFields_0162},
    {0x0163, 13, kFields_0163},
    {0x0164, 15, kFields_0164},
    {0x0165, 2, kFields_0165},
    {0x0166, 3, kFields_0166},
    {0x0167, 7, kFields_0167},
    {0x0168, 5, kFields_0168},
    {0x0169, 12, kFields_0169},
    {0x016A, 2, kFields_016A},
    {0x016B, 61, kFields_016B},
    {0x016C, 15, kFields_016C},
    {0x016D, 5, kFields_016D},
    {0x016E, 1, kFields_016E},
    {0x016F, 3, kFields_016F},
    {0x0170, 1, kFields_0170},
    {0x0171, 6, kFields_0171},
    {0x0172, 5, kFields_0172},
    {0x0173, 76, kFields_0173},
    {0x0174, 3, kFields_0174},
    {0x0175, 1, kFields_0175},
    {0x0176, 3, kFields_0176},
    {0x0177, 1, kFields_0177},
    {0x0178, 2, kFields_0178},
    {0x0179, 1, kFields_0179},
    {0x017A, 2, kFields_017A},
    {0x017B, 2, kFields_017B},
    {0x017C, 19, kFields_017C},
    {0x017D, 4, kFields_017D},
    {0x017E, 1, kFields_017E},
    {0x017F, 2, kFields_017F},
    {0x0180, 161, kFields_0180},
    {0x0181, 39, kFields_0181},
    {0x0182, 1, kFields_0182},
    {0x0183, 4, kFields_0183},
    {0x0184, 6, kFields_0184},
    {0x0185, 3, kFields_0185},
    {0x0186, 2, kFields_0186},
    {0x0187, 42, kFields_0187},
    {0x0188, 3, kFields_0188},
    {0x0189, 10, kFields_0189},
    {0x018A, 4, kFields_018A},
    {0x018B, 2, kFields_018B},
    {0x018C, 40, kFields_018C},
    {0x018D, 4, kFields_018D},
    {0x018E, 2, kFields_018E},
    {0x018F, 7, kFields_018F},
    {0x0190, 11, kFields_0190},
    {0x0191, 1, kFields_0191},
    {0x0192, 7, kFields_0192},
    {0x0193, 4, kFields_0193},
    {0x0194, 25, kFields_0194},
    {0x0195, 13, kFields_0195},
    {0x0196, 2, kFields_0196},
    {0x0197, 5, kFields_0197},
    {0x0198, 3, kFields_0198},
    {0x0199, 3, kFields_0199},
    {0x019A, 53, kFields_019A},
    {0x019B, 19, kFields_019B},
    {0x019C, 15, kFields_019C},
    {0x019D, 5, kFields_019D},
    {0x019E, 3, kFields_019E},
    {0x019F, 4, kFields_019F},
    {0x01A0, 6, kFields_01A0},
    {0x01A1, 5, kFields_01A1},
    {0x01A2, 5, kFields_01A2},
    {0x01A3, 2, kFields_01A3},
    {0x01A4, 2, kFields_01A4},
    {0x01A5, 7, kFields_01A5},
    {0x01A6, 8, kFields_01A6},
    {0x01A7, 76, kFields_01A7},
    {0x01A8, 6, kFields_01A8},
    {0x01A9, 11, kFields_01A9},
    {0x01AA, 6, kFields_01AA},
    {0x01AB, 7, kFields_01AB},
    {0x01AC, 17, kFields_01AC},
    {0x01AD, 23, kFields_01AD},
    {0x01AE, 5, kFields_01AE},
    {0x01AF, 14, kFields_01AF},
    {0x01B0, 13, kFields_01B0},
    {0x01B1, 6, kFields_01B1},
    {0x01B3, 9, kFields_01B3},
    {0x01B4, 27, kFields_01B4},
    {0x01B6, 6, kFields_01B6},
    {0x01B7, 6, kFields_01B7},
    {0x01B8, 43, kFields_01B8},
    {0x01B9, 14, kFields_01B9},
    {0x01BA, 6, kFields_01BA},
    {0x01BB, 9, kFields_01BB},
    {0x01BC, 3, kFields_01BC},
    {0x01BD, 2, kFields_01BD},
    {0x01BE, 3, kFields_01BE},
    {0x01BF, 5, kFields_01BF},
    {0x01C0, 3, kFields_01C0},
    {0x01C1, 2, kFields_01C1},
    {0x01C2, 2, kFields_01C2},
    {0x01C5, 1, kFields_01C5},
    {0x01C6, 6, kFields_01C6},
    {0x01C9, 6, kFields_01C9},
    {0x01CA, 1, kFields_01CA},
    {0x01CB, 4, kFields_01CB},
    {0x01CC, 2, kFields_01CC},
    {0x01CD, 1, kFields_01CD},
    {0x01CF, 10, kFields_01CF},
    {0x01D0, 2, kFields_01D0},
    {0x01D1, 1, kFields_01D1},
    {0x01D3, 1, kFields_01D3},
    {0x01D4, 7, kFields_01D4},
    {0x01D5, 5, kFields_01D5},
    {0x01D6, 4, kFields_01D6},
    {0x01D7, 4, kFields_01D7},
    {0x01D8, 4, kFields_01D8},
    {0x01D9, 2, kFields_01D9},
    {0x01DA, 2, kFields_01DA},
    {0x01DB, 4, kFields_01DB},
    {0x01DC, 2, kFields_01DC},
    {0x01DD, 1, kFields_01DD},
    {0x01DE, 1, kFields_01DE},
    {0x01DF, 5, kFields_01DF},
    {0x01E0, 42, kFields_01E0},
    {0x01E1, 3, kFields_01E1},
    {0x01E2, 3, kFields_01E2},
    {0x01E3, 1, kFields_01E3},
    {0x01E4, 1, kFields_01E4},
    {0x01E5, 1, kFields_01E5},
    {0x01E6, 1, kFields_01E6},
    {0x01E7, 3, kFields_01E7},
    {0x01E8, 5, kFields_01E8},
    {0x01E9, 1, kFields_01E9},
    {0x01EA, 5, kFields_01EA},
    {0x01EB, 2, kFields_01EB},
    {0x01EC, 2, kFields_01EC},
    {0x01ED, 3, kFields_01ED},
    {0x01EE, 4, kFields_01EE},
    {0x01EF, 2, kFields_01EF},
    {0x01F0, 4, kFields_01F0},
    {0x01F1, 1, kFields_01F1},
    {0x01F2, 3, kFields_01F2},
    {0x01F3, 3, kFields_01F3},
    {0x01F4, 2, kFields_01F4},
    {0x01F5, 8, kFields_01F5},
    {0x01F6, 1, kFields_01F6},
    {0x01F7, 5, kFields_01F7},
    {0x01F9, 5, kFields_01F9},
    {0x01FA, 5, kFields_01FA},
    {0x01FB, 6, kFields_01FB},
    {0x01FC, 3, kFields_01FC},
    {0x01FD, 1, kFields_01FD},
    {0x01FE, 1, kFields_01FE},
    {0x01FF, 21, kFields_01FF},
    {0x0200, 3, kFields_0200},
    {0x0201, 1, kFields_0201},
    {0x0202, 1, kFields_0202},
    {0x0203, 1, kFields_0203},
    {0x0204, 2, kFields_0204},
    {0x0206, 1, kFields_0206},
    {0x0207, 1, kFields_0207},
    {0x0208, 8, kFields_0208},
    {0x0209, 1, kFields_0209},
    {0x020A, 2, kFields_020A},
    {0x020B, 1, kFields_020B},
    {0x020E, 1, kFields_020E},
    {0x020F, 4, kFields_020F},
    {0x0210, 2, kFields_0210},
    {0x0211, 2, kFields_0211},
    {0x0212, 1, kFields_0212},
    {0x0213, 6, kFields_0213},
    {0x0214, 1, kFields_0214},
    {0x0215, 2, kFields_0215},
    {0x0216, 6, kFields_0216},
    {0x0217, 3, kFields_0217},
    {0x0218, 2, kFields_0218},
    {0x0219, 3, kFields_0219},
    {0x021A, 4, kFields_021A},
    {0x021B, 1, kFields_021B},
    {0x021C, 1, kFields_021C},
    {0x021D, 9, kFields_021D},
    {0x0220, 3, kFields_0220},
    {0x0223, 2, kFields_0223},
    {0x0224, 2, kFields_0224},
    {0x0225, 2, kFields_0225},
    {0x0226, 2, kFields_0226},
    {0x0227, 4, kFields_0227},
    {0x0228, 8, kFields_0228},
    {0x022A, 1, kFields_022A},
    {0x022C, 2, kFields_022C},
    {0x022D, 7, kFields_022D},
    {0x022E, 5, kFields_022E},
    {0x022F, 3, kFields_022F},
    {0x0230, 6, kFields_0230},
    {0x0231, 9, kFields_0231},
    {0x0232, 1, kFields_0232},
    {0x0233, 1, kFields_0233},
    {0x0234, 4, kFields_0234},
    {0x0235, 9, kFields_0235},
    {0x0238, 2, kFields_0238},
    {0x0239, 6, kFields_0239},
    {0x023B, 2, kFields_023B},
    {0x023C, 5, kFields_023C},
    {0x0241, 1, kFields_0241},
    {0x0242, 1, kFields_0242},
    {0x0243, 6, kFields_0243},
    {0x0244, 4, kFields_0244},
    {0x0245, 2, kFields_0245},
    {0x0247, 1, kFields_0247},
    {0x0248, 9, kFields_0248},
    {0x0249, 1, kFields_0249},
    {0x024A, 1, kFields_024A},
    {0x024B, 8, kFields_024B},
    {0x024C, 2, kFields_024C},
    {0x024D, 7, kFields_024D},
    {0x024F, 7, kFields_024F},
    {0x0250, 2, kFields_0250},
    {0x0252, 3, kFields_0252},
    {0x0253, 2, kFields_0253},
    {0x0254, 3, kFields_0254},
    {0x0256, 1, kFields_0256},
    {0x0257, 1, kFields_0257},
    {0x025D, 1, kFields_025D},
    {0x025E, 1, kFields_025E},
    {0x025F, 10, kFields_025F},
    {0x0260, 14, kFields_0260},
    {0x0261, 5, kFields_0261},
    {0x0263, 1, kFields_0263},
    {0x0264, 2, kFields_0264},
    {0x0266, 1, kFields_0266},
    {0x0267, 1, kFields_0267},
    {0x0268, 1, kFields_0268},
    {0x0269, 2, kFields_0269},
    {0x026A, 1, kFields_026A},
    {0x026B, 1, kFields_026B},
    {0x026C, 1, kFields_026C},
    {0x026D, 2, kFields_026D},
    {0x026E, 1, kFields_026E},
    {0x026F, 3, kFields_026F},
    {0x0270, 3, kFields_0270},
    {0x0271, 4, kFields_0271},
    {0x0272, 2, kFields_0272},
    {0x0273, 4, kFields_0273},
    {0x0274, 3, kFields_0274},
    {0x0275, 2, kFields_0275},
    {0x0276, 2, kFields_0276},
    {0x0277, 2, kFields_0277},
    {0x027A, 1, kFields_027A},
    {0x027B, 2, kFields_027B},
    {0x027C, 1, kFields_027C},
    {0x027D, 4, kFields_027D},
    {0x027E, 6, kFields_027E},
    {0x027F, 1, kFields_027F},
    {0x0280, 2, kFields_0280},
    {0x0281, 2, kFields_0281},
    {0x0282, 1, kFields_0282},
    {0x0284, 2, kFields_0284},
    {0x0286, 1, kFields_0286},
    {0x0287, 15, kFields_0287},
    {0x0288, 4, kFields_0288},
    {0x0289, 3, kFields_0289},
    {0x028A, 13, kFields_028A},
    {0x028B, 1, kFields_028B},
    {0x028C, 2, kFields_028C},
    {0x028D, 1, kFields_028D},
    {0x028E, 5, kFields_028E},
    {0x028F, 3, kFields_028F},
    {0x0290, 3, kFields_0290},
    {0x0291, 5, kFields_0291},
    {0x0293, 2, kFields_0293},
    {0x0294, 3, kFields_0294},
    {0x0295, 1, kFields_0295},
    {0x0297, 2, kFields_0297},
    {0x0298, 2, kFields_0298},
    {0x0299, 3, kFields_0299},
    {0x029A, 1, kFields_029A},
    {0x029C, 1, kFields_029C},
    {0x029D, 5, kFields_029D},
    {0x029E, 6, kFields_029E},
    {0x029F, 6, kFields_029F},
    {0x02A0, 17, kFields_02A0},
    {0x02A2, 2, kFields_02A2},
    {0x02A5, 8, kFields_02A5},
    {0x02A6, 5, kFields_02A6},
    {0x02A7, 10, kFields_02A7},
    {0x02A8, 2, kFields_02A8},
    {0x02A9, 1, kFields_02A9},
    {0x02AA, 3, kFields_02AA},
    {0x02AB, 6, kFields_02AB},
    {0x02AC, 13, kFields_02AC},
    {0x02AD, 20, kFields_02AD},
    {0x02B0, 7, kFields_02B0},
    {0x02B1, 5, kFields_02B1},
    {0x02B2, 1, kFields_02B2},
    {0x02B3, 1, kFields_02B3},
    {0x02B4, 2, kFields_02B4},
    {0x02B5, 4, kFields_02B5},
    {0x02B6, 3, kFields_02B6},
    {0x02B7, 30, kFields_02B7},
    {0x02B8, 18, kFields_02B8},
    {0x02B9, 2, kFields_02B9},
    {0x02BA, 12, kFields_02BA},
    {0x02BB, 5, kFields_02BB},
    {0x02BD, 2, kFields_02BD},
    {0x02BE, 19, kFields_02BE},
    {0x02BF, 3, kFields_02BF},
    {0x02C2, 4, kFields_02C2},
    {0x02C3, 4, kFields_02C3},
    {0x02C4, 2, kFields_02C4},
    {0x02C5, 13, kFields_02C5},
    {0x02C7, 2, kFields_02C7},
    {0x02C9, 3, kFields_02C9},
    {0x02CA, 4, kFields_02CA},
    {0x02CB, 4, kFields_02CB},
    {0x02CC, 5, kFields_02CC},
    {0x02CD, 1, kFields_02CD},
    {0x02CE, 3, kFields_02CE},
    {0x02D0, 2, kFields_02D0},
    {0x02D1, 25, kFields_02D1},
    {0x02D2, 5, kFields_02D2},
    {0x02D3, 4, kFields_02D3},
    {0x02D4, 4, kFields_02D4},
    {0x02D5, 1, kFields_02D5},
    {0x02D6, 2, kFields_02D6},
    {0x02D7, 1, kFields_02D7},
    {0x02D8, 3, kFields_02D8},
    {0x02DA, 3, kFields_02DA},
    {0x02DC, 2, kFields_02DC},
    {0x02DD, 7, kFields_02DD},
    {0x02E0, 5, kFields_02E0},
    {0x02E1, 3, kFields_02E1},
    {0x02E2, 2, kFields_02E2},
    {0x02E4, 4, kFields_02E4},
    {0x02E5, 4, kFields_02E5},
    {0x02E6, 2, kFields_02E6},
    {0x02E7, 8, kFields_02E7},
    {0x02E8, 7, kFields_02E8},
    {0x02E9, 8, kFields_02E9},
    {0x02EA, 5, kFields_02EA},
    {0x02EB, 1, kFields_02EB},
    {0x02EC, 1, kFields_02EC},
    {0x02ED, 4, kFields_02ED},
    {0x02EE, 4, kFields_02EE},
    {0x02F0, 1, kFields_02F0},
    {0x02F1, 2, kFields_02F1},
    {0x02F2, 3, kFields_02F2},
    {0x02F3, 1, kFields_02F3},
    {0x02F4, 14, kFields_02F4},
    {0x02F5, 6, kFields_02F5},
    {0x02F6, 6, kFields_02F6},
    {0x02F7, 4, kFields_02F7},
    {0x02F8, 5, kFields_02F8},
    {0x02F9, 7, kFields_02F9},
    {0x02FA, 1, kFields_02FA},
    {0x02FB, 1, kFields_02FB},
    {0x02FC, 3, kFields_02FC},
    {0x02FD, 2, kFields_02FD},
    {0x02FE, 6, kFields_02FE},
    {0x02FF, 1, kFields_02FF},
    {0x0300, 3, kFields_0300},
    {0x0301, 6, kFields_0301},
    {0x0302, 2, kFields_0302},
    {0x0303, 3, kFields_0303},
    {0x0307, 3, kFields_0307},
    {0x0308, 2, kFields_0308},
    {0x0309, 9, kFields_0309},
    {0x030A, 6, kFields_030A},
    {0x030B, 11, kFields_030B},
    {0x030C, 10, kFields_030C},
    {0x030D, 6, kFields_030D},
    {0x030E, 2, kFields_030E},
    {0x030F, 4, kFields_030F},
    {0x0310, 1, kFields_0310},
    {0x0311, 2, kFields_0311},
    {0x0312, 1, kFields_0312},
    {0x0313, 5, kFields_0313},
    {0x0314, 2, kFields_0314},
    {0x0315, 4, kFields_0315},
    {0x0316, 4, kFields_0316},
    {0x0317, 5, kFields_0317},
    {0x0318, 2, kFields_0318},
    {0x0319, 8, kFields_0319},
    {0x031A, 1, kFields_031A},
    {0x031B, 1, kFields_031B},
    {0x031C, 13, kFields_031C},
    {0x031D, 2, kFields_031D},
    {0x031E, 1, kFields_031E},
    {0x031F, 1, kFields_031F},
    {0x0320, 2, kFields_0320},
    {0x0321, 1, kFields_0321},
    {0x0322, 2, kFields_0322},
    {0x0323, 3, kFields_0323},
    {0x0324, 4, kFields_0324},
    {0x0325, 2, kFields_0325},
    {0x0327, 3, kFields_0327},
    {0x0329, 1, kFields_0329},
    {0x032A, 2, kFields_032A},
    {0x032C, 6, kFields_032C},
    {0x032D, 2, kFields_032D},
    {0x032E, 3, kFields_032E},
    {0x032F, 1, kFields_032F},
    {0x0330, 1, kFields_0330},
    {0x0331, 1, kFields_0331},
    {0x0332, 2, kFields_0332},
    {0x0333, 1, kFields_0333},
    {0x0334, 2, kFields_0334},
    {0x0337, 5, kFields_0337},
    {0x0338, 2, kFields_0338},
    {0x0339, 3, kFields_0339},
    {0x033A, 6, kFields_033A},
    {0x033D, 1, kFields_033D},
    {0x033E, 1, kFields_033E},
    {0x0341, 2, kFields_0341},
    {0x0342, 2, kFields_0342},
    {0x0343, 8, kFields_0343},
    {0x0344, 2, kFields_0344},
    {0x0345, 2, kFields_0345},
    {0x0346, 1, kFields_0346},
    {0x0347, 1, kFields_0347},
    {0x0348, 2, kFields_0348},
    {0x034A, 2, kFields_034A},
    {0x034B, 3, kFields_034B},
    {0x034C, 4, kFields_034C},
    {0x0350, 2, kFields_0350},
    {0x0351, 2, kFields_0351},
    {0x0352, 4, kFields_0352},
    {0x0353, 1, kFields_0353},
    {0x0354, 2, kFields_0354},
    {0x0356, 1, kFields_0356},
    {0x0357, 4, kFields_0357},
    {0x0358, 1, kFields_0358},
    {0x0359, 3, kFields_0359},
    {0x035A, 1, kFields_035A},
    {0x035B, 1, kFields_035B},
    {0x035D, 22, kFields_035D},
    {0x035E, 27, kFields_035E},
    {0x035F, 14, kFields_035F},
    {0x0360, 4, kFields_0360},
    {0x0363, 5, kFields_0363},
    {0x0364, 1, kFields_0364},
    {0x0366, 1, kFields_0366},
    {0x0367, 6, kFields_0367},
    {0x0368, 6, kFields_0368},
    {0x0369, 9, kFields_0369},
    {0x036A, 2, kFields_036A},
    {0x036C, 7, kFields_036C},
    {0x036E, 6, kFields_036E},
    {0x036F, 9, kFields_036F},
    {0x0370, 1, kFields_0370},
    {0x0372, 1, kFields_0372},
    {0x0374, 10, kFields_0374},
    {0x0375, 4, kFields_0375},
    {0x0376, 38, kFields_0376},
    {0x0377, 4, kFields_0377},
    {0x0378, 15, kFields_0378},
    {0x0379, 25, kFields_0379},
    {0x037B, 8, kFields_037B},
    {0x037C, 2, kFields_037C},
    {0x037D, 1, kFields_037D},
    {0x037E, 1, kFields_037E},
    {0x037F, 1, kFields_037F},
    {0x0380, 5, kFields_0380},
    {0x0381, 14, kFields_0381},
    {0x0383, 29, kFields_0383},
    {0x0384, 1, kFields_0384},
    {0x0385, 20, kFields_0385},
    {0x0386, 1, kFields_0386},
    {0x0387, 2, kFields_0387},
    {0x0389, 3, kFields_0389},
    {0x038A, 3, kFields_038A},
    {0x038B, 13, kFields_038B},
    {0x038C, 6, kFields_038C},
    {0x038D, 10, kFields_038D},
    {0x038E, 8, kFields_038E},
    {0x038F, 8, kFields_038F},
    {0x0390, 8, kFields_0390},
    {0x0391, 8, kFields_0391},
    {0x0392, 10, kFields_0392},
    {0x0393, 8, kFields_0393},
    {0x0394, 8, kFields_0394},
    {0x0395, 13, kFields_0395},
    {0x0396, 1, kFields_0396},
    {0x0397, 1, kFields_0397},
    {0x0398, 3, kFields_0398},
    {0x0399, 1, kFields_0399},
    {0x039A, 1, kFields_039A},
    {0x039B, 3, kFields_039B},
    {0x039C, 3, kFields_039C},
    {0x039D, 1, kFields_039D},
    {0x039E, 3, kFields_039E},
    {0x039F, 3, kFields_039F},
    {0x03A0, 3, kFields_03A0},
    {0x03A1, 1, kFields_03A1},
    {0x03A2, 1, kFields_03A2},
    {0x03A3, 1, kFields_03A3},
    {0x03A4, 1, kFields_03A4},
    {0x03A5, 7, kFields_03A5},
    {0x03A6, 1, kFields_03A6},
    {0x03A7, 1, kFields_03A7},
    {0x03A8, 4, kFields_03A8},
    {0x03A9, 1, kFields_03A9},
    {0x03AA, 3, kFields_03AA},
    {0x03AB, 1, kFields_03AB},
    {0x03AC, 2, kFields_03AC},
    {0x03AD, 1, kFields_03AD},
    {0x03AE, 2, kFields_03AE},
    {0x03AF, 3, kFields_03AF},
    {0x03B1, 1, kFields_03B1},
    {0x03B2, 3, kFields_03B2},
    {0x03B3, 2, kFields_03B3},
    {0x03B4, 2, kFields_03B4},
    {0x03B6, 5, kFields_03B6},
    {0x03B7, 1, kFields_03B7},
    {0x03B9, 1, kFields_03B9},
    {0x03BA, 1, kFields_03BA},
    {0x03BC, 2, kFields_03BC},
    {0x03BD, 5, kFields_03BD},
    {0x03BF, 1, kFields_03BF},
    {0x03C0, 3, kFields_03C0},
    {0x03C1, 3, kFields_03C1},
    {0x03C2, 3, kFields_03C2},
    {0x03C3, 1, kFields_03C3},
    {0x03C4, 14, kFields_03C4},
    {0x03C5, 7, kFields_03C5},
    {0x03C6, 26, kFields_03C6},
    {0x03C7, 2, kFields_03C7},
    {0x03C8, 11, kFields_03C8},
    {0x03C9, 1, kFields_03C9},
    {0x03CA, 1, kFields_03CA},
    {0x03CE, 2, kFields_03CE},
    {0x03CF, 2, kFields_03CF},
    {0x03D0, 1, kFields_03D0},
    {0x03D1, 1, kFields_03D1},
    {0x03D2, 3, kFields_03D2},
    {0x03D5, 1, kFields_03D5},
    {0x03D6, 2, kFields_03D6},
    {0x03D7, 1, kFields_03D7},
    {0x03D8, 3, kFields_03D8},
    {0x03D9, 2, kFields_03D9},
    {0x03DA, 4, kFields_03DA},
    {0x03DB, 3, kFields_03DB},
    {0x03DC, 1, kFields_03DC},
    {0x03DD, 1, kFields_03DD},
    {0x03DE, 1, kFields_03DE},
    {0x03DF, 4, kFields_03DF},
    {0x03E0, 6, kFields_03E0},
    {0x03E1, 4, kFields_03E1},
    {0x03E2, 42, kFields_03E2},
    {0x03E3, 108, kFields_03E3},
    {0x03E4, 4, kFields_03E4},
    {0x03E5, 24, kFields_03E5},
    {0x03E6, 4, kFields_03E6},
    {0x03E7, 4, kFields_03E7},
    {0x03E8, 13, kFields_03E8},
    {0x03E9, 6, kFields_03E9},
    {0x03EA, 9, kFields_03EA},
    {0x03EB, 2, kFields_03EB},
    {0x03ED, 13, kFields_03ED},
    {0x03EE, 9, kFields_03EE},
    {0x03EF, 1, kFields_03EF},
    {0x03F3, 1, kFields_03F3},
    {0x03F5, 2, kFields_03F5},
    {0x03F6, 4, kFields_03F6},
    {0x03F7, 1, kFields_03F7},
    {0x03F8, 42, kFields_03F8},
    {0x03F9, 3, kFields_03F9},
    {0x03FA, 148, kFields_03FA},
    {0x03FB, 148, kFields_03FB},
    {0x03FC, 6, kFields_03FC},
    {0x03FD, 1, kFields_03FD},
    {0x03FE, 17, kFields_03FE},
    {0x03FF, 7, kFields_03FF},
    {0x0400, 2, kFields_0400},
    {0x0401, 21, kFields_0401},
    {0x0402, 1, kFields_0402},
    {0x0403, 1, kFields_0403},
    {0x0404, 4, kFields_0404},
    {0x0405, 1, kFields_0405},
    {0x0406, 2, kFields_0406},
    {0x0407, 11, kFields_0407},
    {0x0408, 5, kFields_0408},
    {0x0409, 5, kFields_0409},
    {0x040A, 5, kFields_040A},
    {0x040B, 3, kFields_040B},
    {0x040C, 5, kFields_040C},
    {0x040D, 7, kFields_040D},
    {0x040E, 2, kFields_040E},
    {0x040F, 2, kFields_040F},
    {0x0410, 3, kFields_0410},
    {0x0411, 2, kFields_0411},
    {0x0413, 3, kFields_0413},
    {0x0414, 3, kFields_0414},
    {0x0415, 10, kFields_0415},
    {0x0416, 3, kFields_0416},
    {0x0417, 1, kFields_0417},
    {0x0418, 1, kFields_0418},
    {0x0419, 2, kFields_0419},
    {0x041A, 1, kFields_041A},
    {0x041B, 10, kFields_041B},
    {0x041C, 1, kFields_041C},
    {0x041D, 1, kFields_041D},
    {0x041E, 1, kFields_041E},
    {0x041F, 2, kFields_041F},
    {0x0420, 8, kFields_0420},
    {0x0421, 1, kFields_0421},
    {0x0422, 4, kFields_0422},
    {0x0423, 1, kFields_0423},
    {0x0424, 1, kFields_0424},
    {0x0425, 1, kFields_0425},
    {0x0426, 1, kFields_0426},
    {0x0427, 1, kFields_0427},
    {0x0428, 1, kFields_0428},
    {0x042A, 2, kFields_042A},
    {0x042B, 2, kFields_042B},
    {0x042C, 2, kFields_042C},
    {0x042D, 2, kFields_042D},
    {0x042E, 1, kFields_042E},
    {0x042F, 1, kFields_042F},
    {0x0430, 1, kFields_0430},
    {0x0431, 1, kFields_0431},
    {0x0432, 1, kFields_0432},
    {0x0433, 1, kFields_0433},
    {0x0434, 1, kFields_0434},
    {0x0435, 1, kFields_0435},
    {0x0436, 1, kFields_0436},
    {0x0437, 4, kFields_0437},
    {0x0438, 3, kFields_0438},
    {0x043A, 7, kFields_043A},
    {0x043C, 2, kFields_043C},
    {0x043D, 1, kFields_043D},
    {0x043E, 1, kFields_043E},
    {0x043F, 1, kFields_043F},
    {0x0440, 1, kFields_0440},
    {0x0441, 1, kFields_0441},
    {0x0442, 3, kFields_0442},
    {0x0443, 1, kFields_0443},
    {0x0444, 2, kFields_0444},
    {0x0445, 3, kFields_0445},
    {0x0446, 1, kFields_0446},
    {0x0448, 1, kFields_0448},
    {0x0449, 1, kFields_0449},
    {0x044A, 1, kFields_044A},
    {0x044B, 3, kFields_044B},
    {0x044E, 3, kFields_044E},
    {0x044F, 3, kFields_044F},
    {0x0450, 4, kFields_0450},
    {0x0451, 4, kFields_0451},
    {0x0452, 4, kFields_0452},
    {0x0453, 8, kFields_0453},
    {0x0454, 16, kFields_0454},
    {0x0455, 71, kFields_0455},
    {0x0456, 6, kFields_0456},
    {0x0457, 1, kFields_0457},
    {0x0458, 2, kFields_0458},
    {0x0459, 2, kFields_0459},
    {0x045A, 6, kFields_045A},
    {0x045B, 4, kFields_045B},
    {0x045C, 9, kFields_045C},
    {0x045D, 3, kFields_045D},
    {0x045E, 3, kFields_045E},
    {0x0460, 4, kFields_0460},
    {0x0461, 5, kFields_0461},
    {0x0462, 13, kFields_0462},
    {0x0463, 2, kFields_0463},
    {0x0464, 22, kFields_0464},
    {0x0465, 5, kFields_0465},
    {0x0466, 7, kFields_0466},
    {0x0467, 44, kFields_0467},
    {0x0468, 7, kFields_0468},
    {0x0469, 6, kFields_0469},
    {0x046A, 8, kFields_046A},
    {0x046B, 7, kFields_046B},
    {0x046C, 11, kFields_046C},
    {0x046D, 17, kFields_046D},
    {0x046E, 25, kFields_046E},
    {0x046F, 2, kFields_046F},
    {0x0470, 12, kFields_0470},
    {0x0471, 2, kFields_0471},
    {0x0472, 1, kFields_0472},
    {0x0473, 2, kFields_0473},
    {0x0474, 2, kFields_0474},
    {0x0475, 2, kFields_0475},
    {0x0476, 9, kFields_0476},
    {0x0477, 1, kFields_0477},
    {0x0478, 2, kFields_0478},
    {0x0479, 1, kFields_0479},
    {0x047A, 5, kFields_047A},
    {0x047B, 12, kFields_047B},
    {0x047C, 9, kFields_047C},
    {0x047D, 11, kFields_047D},
    {0x047E, 2, kFields_047E},
    {0x047F, 10, kFields_047F},
    {0x0480, 2, kFields_0480},
    {0x0481, 4, kFields_0481},
    {0x0482, 3, kFields_0482},
    {0x0483, 2, kFields_0483},
    {0x0484, 2, kFields_0484},
    {0x0485, 2, kFields_0485},
    {0x0486, 1, kFields_0486},
    {0x0487, 4, kFields_0487},
    {0x0488, 1, kFields_0488},
    {0x0489, 3, kFields_0489},
    {0x048A, 2, kFields_048A},
    {0x048B, 7, kFields_048B},
    {0x048C, 2, kFields_048C},
    {0x048D, 138, kFields_048D},
    {0x048E, 3, kFields_048E},
    {0x048F, 7, kFields_048F},
    {0x0490, 1, kFields_0490},
    {0x0491, 5, kFields_0491},
    {0x0492, 3, kFields_0492},
    {0x0493, 39, kFields_0493},
    {0x0494, 20, kFields_0494},
    {0x0495, 2, kFields_0495},
    {0x0496, 3, kFields_0496},
    {0x0497, 1, kFields_0497},
    {0x0498, 7, kFields_0498},
    {0x0499, 2, kFields_0499},
    {0x049A, 19, kFields_049A},
    {0x049B, 1, kFields_049B},
    {0x049C, 3, kFields_049C},
    {0x049D, 38, kFields_049D},
    {0x049E, 6, kFields_049E},
    {0x049F, 3, kFields_049F},
    {0x04A0, 2, kFields_04A0},
    {0x04A1, 5, kFields_04A1},
    {0x04A2, 5, kFields_04A2},
    {0x04A3, 2, kFields_04A3},
    {0x04A4, 4, kFields_04A4},
    {0x04A5, 5, kFields_04A5},
    {0x04A6, 1, kFields_04A6},
    {0x04A7, 1, kFields_04A7},
    {0x04A8, 3, kFields_04A8},
    {0x04A9, 2, kFields_04A9},
    {0x04AA, 3, kFields_04AA},
    {0x04AB, 11, kFields_04AB},
    {0x04AC, 3, kFields_04AC},
    {0x04AD, 1, kFields_04AD},
    {0x04AE, 1, kFields_04AE},
    {0x04AF, 2, kFields_04AF},
    {0x04B0, 1, kFields_04B0},
    {0x04B1, 2, kFields_04B1},
    {0x04B2, 1, kFields_04B2},
    {0x04B3, 3, kFields_04B3},
    {0x04B4, 2, kFields_04B4},
    {0x04B5, 1, kFields_04B5},
    {0x04B6, 5, kFields_04B6},
    {0x04B7, 3, kFields_04B7},
    {0x04B8, 2, kFields_04B8},
    {0x04B9, 2, kFields_04B9},
    {0x04BA, 3, kFields_04BA},
    {0x04BB, 6, kFields_04BB},
    {0x04BC, 4, kFields_04BC},
    {0x04BD, 1, kFields_04BD},
    {0x04BE, 1, kFields_04BE},
    {0x04BF, 1, kFields_04BF},
    {0x04C0, 5, kFields_04C0},
    {0x04C2, 1, kFields_04C2},
    {0x04C3, 1, kFields_04C3},
    {0x04C4, 7, kFields_04C4},
    {0x04C5, 3, kFields_04C5},
    {0x04C6, 2, kFields_04C6},
    {0x04C7, 1, kFields_04C7},
    {0x04C8, 2, kFields_04C8},
    {0x04C9, 1, kFields_04C9},
    {0x04CA, 1, kFields_04CA},
    {0x04CB, 2, kFields_04CB},
    {0x04CC, 1, kFields_04CC},
    {0x04CD, 2, kFields_04CD},
    {0x04CE, 4, kFields_04CE},
    {0x04CF, 1, kFields_04CF},
    {0x04D0, 2, kFields_04D0},
    {0x04D2, 4, kFields_04D2},
    {0x04D3, 6, kFields_04D3},
    {0x04D4, 2, kFields_04D4},
    {0x04D5, 3, kFields_04D5},
    {0x04D6, 2, kFields_04D6},
    {0x04D7, 26, kFields_04D7},
    {0x04D8, 5, kFields_04D8},
    {0x04D9, 1, kFields_04D9},
    {0x04DA, 1, kFields_04DA},
    {0x04DB, 3, kFields_04DB},
    {0x04DC, 3, kFields_04DC},
    {0x04DD, 3, kFields_04DD},
    {0x04DE, 11, kFields_04DE},
    {0x04DF, 9, kFields_04DF},
    {0x04E0, 1, kFields_04E0},
    {0x04E1, 1, kFields_04E1},
    {0x04E2, 6, kFields_04E2},
    {0x04E3, 1, kFields_04E3},
    {0x04E4, 3, kFields_04E4},
    {0x04E5, 2, kFields_04E5},
    {0x04E6, 2, kFields_04E6},
    {0x04E7, 3, kFields_04E7},
    {0x04E8, 3, kFields_04E8},
    {0x04E9, 3, kFields_04E9},
    {0x04EA, 5, kFields_04EA},
    {0x04EB, 2, kFields_04EB},
    {0x04EC, 2, kFields_04EC},
    {0x04ED, 2, kFields_04ED},
    {0x04EE, 3, kFields_04EE},
    {0x04EF, 3, kFields_04EF},
    {0x04F0, 4, kFields_04F0},
    {0x04F1, 5, kFields_04F1},
    {0x04F2, 9, kFields_04F2},
    {0x04F3, 1, kFields_04F3},
    {0x04F4, 17, kFields_04F4},
    {0x04F5, 4, kFields_04F5},
    {0x04F6, 1, kFields_04F6},
    {0x04F7, 2, kFields_04F7},
    {0x04F8, 2, kFields_04F8},
    {0x04FA, 1, kFields_04FA},
    {0x04FB, 1, kFields_04FB},
    {0x04FC, 4, kFields_04FC},
    {0x04FD, 2, kFields_04FD},
    {0x04FE, 2, kFields_04FE},
    {0x04FF, 15, kFields_04FF},
    {0x0500, 1, kFields_0500},
    {0x0501, 3, kFields_0501},
    {0x0502, 5, kFields_0502},
    {0x0503, 22, kFields_0503},
    {0x0504, 10, kFields_0504},
    {0x0505, 9, kFields_0505},
    {0x0506, 26, kFields_0506},
    {0x0507, 3, kFields_0507},
    {0x0508, 3, kFields_0508},
    {0x0509, 1, kFields_0509},
    {0x050A, 1, kFields_050A},
    {0x050B, 6, kFields_050B},
    {0x050C, 3, kFields_050C},
    {0x050D, 2, kFields_050D},
    {0x050E, 9, kFields_050E},
    {0x0511, 1, kFields_0511},
    {0x0512, 1, kFields_0512},
    {0x0513, 3, kFields_0513},
    {0x0514, 2, kFields_0514},
    {0x0515, 6, kFields_0515},
    {0x0516, 1, kFields_0516},
    {0x0517, 4, kFields_0517},
    {0x0518, 7, kFields_0518},
    {0x0519, 1, kFields_0519},
    {0x051A, 6, kFields_051A},
    {0x051B, 1, kFields_051B},
    {0x051C, 4, kFields_051C},
    {0x051D, 3, kFields_051D},
    {0x051E, 4, kFields_051E},
    {0x051F, 4, kFields_051F},
    {0x0520, 4, kFields_0520},
    {0x0521, 8, kFields_0521},
    {0x0522, 3, kFields_0522},
    {0x0523, 5, kFields_0523},
    {0x0524, 450, kFields_0524},
    {0x0525, 1, kFields_0525},
    {0x0527, 1, kFields_0527},
    {0x0528, 3, kFields_0528},
    {0x0529, 2, kFields_0529},
    {0x052A, 2, kFields_052A},
    {0x052C, 1, kFields_052C},
    {0x052D, 7, kFields_052D},
    {0x052E, 1, kFields_052E},
    {0x052F, 6, kFields_052F},
    {0x0530, 1, kFields_0530},
    {0x0531, 2, kFields_0531},
    {0x0532, 1, kFields_0532},
    {0x0533, 5, kFields_0533},
    {0x0534, 7, kFields_0534},
    {0x0535, 4, kFields_0535},
    {0x0536, 2, kFields_0536},
    {0x0537, 4, kFields_0537},
    {0x0538, 6, kFields_0538},
    {0x0539, 1, kFields_0539},
    {0x053A, 3, kFields_053A},
    {0x053B, 2, kFields_053B},
    {0x053C, 1, kFields_053C},
    {0x053D, 2, kFields_053D},
    {0x053E, 3, kFields_053E},
    {0x053F, 3, kFields_053F},
    {0x0540, 4, kFields_0540},
    {0x0541, 2, kFields_0541},
    {0x0542, 2, kFields_0542},
    {0x0543, 2, kFields_0543},
    {0x0544, 2, kFields_0544},
    {0x0546, 2, kFields_0546},
    {0x0547, 2, kFields_0547},
    {0x0548, 1, kFields_0548},
    {0x0549, 2, kFields_0549},
    {0x054B, 1, kFields_054B},
    {0x054C, 2, kFields_054C},
    {0x054D, 2, kFields_054D},
    {0x054E, 1, kFields_054E},
    {0x0551, 1, kFields_0551},
    {0x0552, 1, kFields_0552},
    {0x0553, 1, kFields_0553},
    {0x0555, 1, kFields_0555},
    {0x0556, 2, kFields_0556},
    {0x0557, 1, kFields_0557},
    {0x0558, 1, kFields_0558},
    {0x0559, 2, kFields_0559},
    {0x055A, 1, kFields_055A},
    {0x055B, 1, kFields_055B},
    {0x055C, 2, kFields_055C},
    {0x055E, 1, kFields_055E},
    {0x055F, 3, kFields_055F},
    {0x0560, 1, kFields_0560},
    {0x0561, 1, kFields_0561},
    {0x0562, 14, kFields_0562},
    {0x0563, 1, kFields_0563},
    {0x0564, 3, kFields_0564},
    {0x0565, 2, kFields_0565},
    {0x0566, 7, kFields_0566},
    {0x0567, 3, kFields_0567},
    {0x0568, 2, kFields_0568},
    {0x0569, 4, kFields_0569},
    {0x056A, 3, kFields_056A},
    {0x056B, 2, kFields_056B},
    {0x056C, 4, kFields_056C},
    {0x056D, 3, kFields_056D},
    {0x056E, 4, kFields_056E},
    {0x056F, 1, kFields_056F},
    {0x0570, 3, kFields_0570},
    {0x0571, 2, kFields_0571},
    {0x0572, 3, kFields_0572},
    {0x0573, 4, kFields_0573},
    {0x0574, 2, kFields_0574},
    {0x0575, 1, kFields_0575},
    {0x0576, 1, kFields_0576},
    {0x0577, 1, kFields_0577},
    {0x0579, 1, kFields_0579},
    {0x057A, 6, kFields_057A},
    {0x057B, 19, kFields_057B},
    {0x057C, 2, kFields_057C},
    {0x057D, 4, kFields_057D},
    {0x057E, 6, kFields_057E},
    {0x057F, 3, kFields_057F},
    {0x0580, 1, kFields_0580},
    {0x0581, 13, kFields_0581},
    {0x0582, 6, kFields_0582},
    {0x0583, 2, kFields_0583},
    {0x0585, 1, kFields_0585},
    {0x0586, 1, kFields_0586},
    {0x0587, 30, kFields_0587},
    {0x0588, 1, kFields_0588},
    {0x0589, 3, kFields_0589},
    {0x058A, 4, kFields_058A},
    {0x058B, 2, kFields_058B},
    {0x058C, 1, kFields_058C},
    {0x058D, 9, kFields_058D},
    {0x058E, 1, kFields_058E},
    {0x058F, 1, kFields_058F},
    {0x0590, 8, kFields_0590},
    {0x0591, 2, kFields_0591},
    {0x0592, 2, kFields_0592},
    {0x0593, 3, kFields_0593},
    {0x0594, 22, kFields_0594},
    {0x0595, 6, kFields_0595},
    {0x0596, 5, kFields_0596},
    {0x0597, 3, kFields_0597},
    {0x0598, 4, kFields_0598},
    {0x0599, 7, kFields_0599},
    {0x059A, 2, kFields_059A},
    {0x059B, 4, kFields_059B},
    {0x059C, 5, kFields_059C},
    {0x059D, 3, kFields_059D},
    {0x059E, 3, kFields_059E},
    {0x059F, 2, kFields_059F},
    {0x05A0, 4, kFields_05A0},
    {0x05A1, 3, kFields_05A1},
    {0x05A2, 3, kFields_05A2},
    {0x05A3, 3, kFields_05A3},
    {0x05A4, 5, kFields_05A4},
    {0x05A5, 1, kFields_05A5},
    {0x05A7, 5, kFields_05A7},
    {0x05A8, 2, kFields_05A8},
    {0x05A9, 4, kFields_05A9},
    {0x05AA, 2, kFields_05AA},
    {0x05AB, 2, kFields_05AB},
    {0x05AC, 4, kFields_05AC},
    {0x05AD, 5, kFields_05AD},
    {0x05AE, 1, kFields_05AE},
    {0x05AF, 3, kFields_05AF},
    {0x05B0, 4, kFields_05B0},
    {0x05B1, 5, kFields_05B1},
    {0x05B2, 2, kFields_05B2},
    {0x05B3, 8, kFields_05B3},
    {0x05B4, 3, kFields_05B4},
    {0x05B5, 2, kFields_05B5},
    {0x05B6, 1, kFields_05B6},
    {0x05B7, 5, kFields_05B7},
    {0x05B8, 2, kFields_05B8},
    {0x05B9, 2, kFields_05B9},
    {0x05BA, 2, kFields_05BA},
    {0x05BB, 2, kFields_05BB},
    {0x05BC, 4, kFields_05BC},
    {0x05BD, 8, kFields_05BD},
    {0x05BE, 3, kFields_05BE},
    {0x05BF, 4, kFields_05BF},
    {0x05C0, 8, kFields_05C0},
    {0x05C1, 3, kFields_05C1},
    {0x05C2, 10, kFields_05C2},
    {0x05C3, 3, kFields_05C3},
    {0x05C4, 5, kFields_05C4},
    {0x05C5, 8, kFields_05C5},
    {0x05C6, 5, kFields_05C6},
    {0x05C7, 4, kFields_05C7},
    {0x05C8, 6, kFields_05C8},
    {0x05C9, 4, kFields_05C9},
    {0x05CA, 17, kFields_05CA},
    {0x05CB, 8, kFields_05CB},
    {0x05CC, 4, kFields_05CC},
    {0x05CD, 2, kFields_05CD},
    {0x05CE, 7, kFields_05CE},
    {0x05CF, 2, kFields_05CF},
    {0x05D0, 3, kFields_05D0},
    {0x05D1, 1, kFields_05D1},
    {0x05D2, 2, kFields_05D2},
    {0x05D3, 1, kFields_05D3},
    {0x05D4, 2, kFields_05D4},
    {0x05D5, 14, kFields_05D5},
    {0x05D6, 3, kFields_05D6},
    {0x05D7, 2, kFields_05D7},
    {0x05D8, 3, kFields_05D8},
    {0x05D9, 3, kFields_05D9},
    {0x05DA, 15, kFields_05DA},
    {0x05DB, 2, kFields_05DB},
    {0x05DC, 2, kFields_05DC},
    {0x05DD, 3, kFields_05DD},
    {0x05DE, 2, kFields_05DE},
    {0x05DF, 9, kFields_05DF},
    {0x05E0, 2, kFields_05E0},
    {0x05E1, 2, kFields_05E1},
    {0x05E2, 9, kFields_05E2},
    {0x05E3, 2, kFields_05E3},
    {0x05E4, 6, kFields_05E4},
    {0x05E7, 3, kFields_05E7},
    {0x05E8, 2, kFields_05E8},
    {0x05E9, 1, kFields_05E9},
    {0x05EA, 1, kFields_05EA},
    {0x05EB, 3, kFields_05EB},
    {0x05EC, 2, kFields_05EC},
    {0x05ED, 4, kFields_05ED},
    {0x05EE, 5, kFields_05EE},
    {0x05EF, 2, kFields_05EF},
    {0x05F0, 2, kFields_05F0},
    {0x05F1, 3, kFields_05F1},
    {0x05F2, 3, kFields_05F2},
    {0x05F3, 3, kFields_05F3},
    {0x05F4, 5, kFields_05F4},
    {0x05F5, 2, kFields_05F5},
    {0x05F6, 2, kFields_05F6},
    {0x05F7, 3, kFields_05F7},
    {0x05F8, 5, kFields_05F8},
    {0x05F9, 3, kFields_05F9},
    {0x05FA, 5, kFields_05FA},
    {0x05FB, 1, kFields_05FB},
    {0x05FC, 1, kFields_05FC},
    {0x05FD, 6, kFields_05FD},
    {0x05FE, 1, kFields_05FE},
    {0x05FF, 2, kFields_05FF},
    {0x0600, 1, kFields_0600},
    {0x0601, 2, kFields_0601},
    {0x0602, 2, kFields_0602},
    {0x0603, 2, kFields_0603},
    {0x0604, 4, kFields_0604},
    {0x0605, 4, kFields_0605},
    {0x0606, 1, kFields_0606},
    {0x0607, 26, kFields_0607},
    {0x0608, 7, kFields_0608},
    {0x0609, 3, kFields_0609},
    {0x060A, 32, kFields_060A},
    {0x060B, 5, kFields_060B},
    {0x060C, 45, kFields_060C},
    {0x060D, 3, kFields_060D},
    {0x060E, 6, kFields_060E},
    {0x060F, 2, kFields_060F},
    {0x0610, 5, kFields_0610},
    {0x0611, 4, kFields_0611},
    {0x0612, 2, kFields_0612},
    {0x0613, 2, kFields_0613},
    {0x0614, 19, kFields_0614},
    {0x0615, 1, kFields_0615},
    {0x0616, 2, kFields_0616},
    {0x0617, 1, kFields_0617},
    {0x0618, 9, kFields_0618},
    {0x0619, 28, kFields_0619},
    {0x061A, 3, kFields_061A},
    {0x061B, 4, kFields_061B},
    {0x061C, 2, kFields_061C},
    {0x061D, 1, kFields_061D},
    {0x061E, 2, kFields_061E},
    {0x061F, 9, kFields_061F},
    {0x0620, 2, kFields_0620},
    {0x0621, 2, kFields_0621},
    {0x0622, 2, kFields_0622},
    {0x0623, 3, kFields_0623},
    {0x0624, 2, kFields_0624},
    {0x0625, 11, kFields_0625},
};

inline constexpr int kStructCount = 1427;

// The struct carrying a material constant override. It is the same shape the
// MAT entry's parameter table stores per parameter (see MaterialParser.h's
// MatParam): a name and a float value. MaterialConstantName is a StringHash,
// which is exactly why the WAD holds a nameHash and no text.
inline constexpr uint16_t kMaterialConstantStructId = 0x0269;

// Linear lookups over the tables above. Both return nullptr when not found.
const Struct* FindStruct(uint16_t id);
const Field*  FindField(uint16_t structId, const char* name);

} // namespace Onyx::Gowr::SmSchema
