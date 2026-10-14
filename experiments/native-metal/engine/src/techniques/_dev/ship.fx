technique ShipCannonAngles
{
    pass p0
    {
        ZEnable = false;
        Lighting = false;
        CullMode = none;
        AlphaTestEnable = false;
        AlphaBlendEnable = true;

        ColorOp[0] = selectarg1;
        ColorArg1[0] = diffuse;

        AlphaOp[0] = selectarg1;
        AlphaArg1[0] = diffuse;

        ColorOp[1] = disable;
    }
}

// Surface-only alpha-blended aim contacts. Volume is a dedicated Metal density pass.
// Explicit depth state is required:
// inherited HUD state is not a reliable occlusion contract.

technique ShipAimFootprint
{
    pass p0
    {
        ZEnable = true;
        ZWriteEnable = false;
        Lighting = false;
        CullMode = none;
        AlphaTestEnable = false;
        AlphaBlendEnable = true;
        SrcBlend = srcalpha;
        DestBlend = invsrcalpha;
        ColorOp[0] = selectarg1;
        ColorArg1[0] = diffuse;
        AlphaOp[0] = selectarg1;
        AlphaArg1[0] = diffuse;
        ColorOp[1] = disable;
    }
}

// Subtle centered orientation plus; explicit user request. No range/status HUD.
technique ShipAimReticle
{
    pass p0
    {
        ZEnable = false;
        ZWriteEnable = false;
        Lighting = false;
        CullMode = none;
        AlphaTestEnable = false;
        AlphaBlendEnable = true;
        SrcBlend = srcalpha;
        DestBlend = invsrcalpha;
        ColorOp[0] = selectarg1;
        ColorArg1[0] = diffuse;
        AlphaOp[0] = selectarg1;
        AlphaArg1[0] = diffuse;
        ColorOp[1] = disable;
    }
}
