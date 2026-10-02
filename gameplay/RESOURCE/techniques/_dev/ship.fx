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

// Manual aim plumage: depth-tested counterparts of ShipCannonAngles.
// ShipAimArc: 1px lines with vertex diffuse + alpha, occluded by hulls/sails/land.
// ShipAimVolume: soft smoke-ribbon triangles, blended, no depth writes.
technique ShipAimArc
{
    pass p0
    {
        Lighting = false;
        CullMode = none;
        AlphaTestEnable = false;
        AlphaBlendEnable = true;
        SrcBlend = srcalpha;
        DestBlend = invsrcalpha;
        ZWriteEnable = false;

        ColorOp[0] = selectarg1;
        ColorArg1[0] = diffuse;

        AlphaOp[0] = selectarg1;
        AlphaArg1[0] = diffuse;

        ColorOp[1] = disable;
    }
}

technique ShipAimVolume
{
    pass p0
    {
        Lighting = false;
        CullMode = none;
        AlphaTestEnable = false;
        AlphaBlendEnable = true;
        SrcBlend = srcalpha;
        DestBlend = invsrcalpha;
        ZWriteEnable = false;

        ColorOp[0] = selectarg1;
        ColorArg1[0] = diffuse;

        AlphaOp[0] = selectarg1;
        AlphaArg1[0] = diffuse;

        ColorOp[1] = disable;
    }
}
