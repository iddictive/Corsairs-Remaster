#pragma once

#include "matrix.h"
#include "animation.h"
#include "math3d/quaternion.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

namespace roundhouse
{
// Skinning palettes contain world-space joint transforms. Blend local joint
// transforms instead, so a turning torso carries its shoulders and elbows.
inline void BlendPose(const Animation &animation, const std::vector<CMatrix> &from,
                      std::vector<CMatrix> &to, float amount, std::vector<CMatrix> &scratch)
{
    const auto count = to.size();
    if (from.size() != count || count != static_cast<std::size_t>(animation.GetNumBones())) return;
    if (amount <= 0.f) { to = from; return; }
    if (amount >= 1.f) return;
    scratch.resize(count * 3);
    CMatrix *fromWorld = scratch.data(), *toWorld = fromWorld + count, *blendedWorld = toWorld + count;
    for (std::size_t i = 0; i < count; ++i)
    {
        CMatrix bind = animation.GetInverseBindMatrix(static_cast<int>(i));
        bind.Transposition();
        fromWorld[i] = bind * from[i];
        toWorld[i] = bind * to[i];
        CMatrix a = fromWorld[i], b = toWorld[i];
        const int parent = animation.GetBoneParent(static_cast<int>(i));
        if (parent >= 0)
        {
            CMatrix inverse = fromWorld[parent]; inverse.Transposition(); a = a * inverse;
            inverse = toWorld[parent]; inverse.Transposition(); b = b * inverse;
        }
        Matrix first, second, result;
        std::copy(std::begin(a.matrix), std::end(a.matrix), first.matrix);
        std::copy(std::begin(b.matrix), std::end(b.matrix), second.matrix);
        Quaternion rotation;
        rotation.SLerp(Quaternion(first), Quaternion(second), amount).GetMatrix(result);
        CMatrix local; local = result;
        local.Pos() = a.v.pos + (b.v.pos - a.v.pos) * amount;
        blendedWorld[i] = parent >= 0 ? local * blendedWorld[parent] : local;
        to[i] = animation.GetInverseBindMatrix(static_cast<int>(i)) * blendedWorld[i];
    }
}
// Native CPU/Metal skinning negates model-local output X after the raw AN
// palette. A joint must be conjugated by that reflection (F * J * F), so its
// origin follows the rendered mesh while its orientation remains proper.
inline CMatrix MirrorJoint(CMatrix joint)
{
    for (const int index : {1, 2, 3, 4, 8, 12})
        joint.matrix[index] = -joint.matrix[index];
    return joint;
}

inline CVECTOR MirrorOffset(const CVECTOR &offset)
{
    return CVECTOR(-offset.x, offset.y, offset.z);
}

// CMatrix A * B applies A, then B. Reflection belongs inside the model
// transform; reflecting the final world matrix would also mirror the location.
inline CMatrix AnimationJointWorld(CMatrix inverseBind, const CMatrix &skinPalette, const CMatrix &model)
{
    inverseBind.Transposition();
    return MirrorJoint(inverseBind * skinPalette) * model;
}

inline CMatrix WorldToSkinPalette(const CMatrix &inverseBind, const CMatrix &jointWorld,
                                 const CMatrix &inverseModel)
{
    // Equivalent to inverseBind * MirrorJoint(jointWorld * inverseModel),
    // retaining the original multiplication order on the per-bone hot path:
    // F * inverseModel * jointWorld * F * inverseBind (column-vector notation).
    CMatrix reflectedBind = inverseBind, reflectedModel = inverseModel;
    for (const int index : {0, 4, 8, 12})
    {
        reflectedBind.matrix[index] = -reflectedBind.matrix[index];
        reflectedModel.matrix[index] = -reflectedModel.matrix[index];
    }
    return reflectedBind * jointWorld * reflectedModel;
}

inline CVECTOR JointPointWorld(CMatrix jointWorld, const CVECTOR &authoredOffset)
{
    return jointWorld * MirrorOffset(authoredOffset);
}
} // namespace roundhouse
