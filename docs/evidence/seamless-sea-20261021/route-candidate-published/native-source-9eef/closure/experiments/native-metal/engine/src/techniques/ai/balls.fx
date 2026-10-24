technique Cannonballs
{
    pass p0
    {
        ZWriteEnable = false;
        Lighting = false;
        CullMode = none;
        AlphaTestEnable = true;
        AlphaBlendEnable = true;

        ColorOp[0] = selectarg1;
        ColorArg1[0] = texture;

        AlphaOp[0] = selectarg1;
        AlphaArg1[0] = texture;

        ColorOp[1] = disable;
    }
}

technique DartTracer
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
