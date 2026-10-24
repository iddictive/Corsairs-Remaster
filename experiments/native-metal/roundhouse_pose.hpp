#pragma once

#include "matrix.h"

#include <initializer_list>

namespace roundhouse
{
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
