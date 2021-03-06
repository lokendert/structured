uniform vec3 weights;
uniform int shaderOut;
varying vec4 normal;
varying vec3 L;
varying vec3 E;
varying vec3 H;
varying vec3 VaryingTexCoord[4];
varying float ao;

uniform sampler2D theTexture;
vec4 jetColorMap(float val) {
  val= clamp(val,0.0,1.0);

  vec4 jet;
        jet.x = min(4.0 * val - 1.5,-4.0 * val + 4.5) ;
        jet.y = min(4.0 * val - 0.5,-4.0 * val + 3.5) ;
        jet.z = min(4.0 * val + 0.5,-4.0 * val + 2.5) ;


        jet.x = clamp(jet.x, 0.0, 1.0);
        jet.y = clamp(jet.y, 0.0, 1.0);
        jet.z = clamp(jet.z, 0.0, 1.0);
	jet.w = 1.0;
        return jet;
}

vec4 HSV_to_RGB (vec4 hsv){
  vec4 color;
  float f,p,q,t;
  float h,s,v;
  float r=0.0,g=0.0,b=0.0;
  float i;
  if (hsv[1] == 0.0){
    if (hsv[2] != 0.0){
      color.x = hsv[2];
    }
  }
  else{
    h = hsv.x * 360.0;
    s = hsv.y;
    v = hsv.z;
    if (h == 360.0) {
      h=0.0;
    }
    h /=60.0;
    i = floor (h);
    f = h-i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * f));
    t = v * (1.0 - (s * (1.0 -f)));
    if (i == 0.0){
      r = v;
      g = t;
      b = p;
    }
    else if (i == 1.0){
      r = q;
      g = v;
      b = p;
    }
    else if (i == 2.0){
      r = p;
      g = v;
      b = t;
    }
    else if (i == 3.0) {
      r = p;
      g = q;
      b = v;
    }
    else if (i == 4.0) {
      r = t;
      g = p;
      b = v;
    }
    else if (i == 5.0) {
      r = v;
      g = p;
      b = q;
    }
    color.r = r;
    color.g = g;
    color.b = b;
    color.w = hsv.w;
  }
  return color;
}

vec4 rainbowColorMap(float hue) {
  return HSV_to_RGB(vec4(hue, 1.0f, 1.0f,1.0));
}

vec4 freq3Blend(vec3 Cb){
  vec3 mipmapL = vec3(0,2,4);
  float rmax=0.70710678;
  vec3 WSum =vec3(0.0,0.0,0.0);

  vec4 outP;
  vec4 outComp[3];
  outComp[0]=vec4(0.0,0.0,0.0,0.0);
  outComp[1]=vec4(0.0,0.0,0.0,0.0);
  outComp[2]=vec4(0.0,0.0,0.0,0.0);

  bool validPix=false;
  for(int j=0; j < 3; j++){
    for(int i=0;i<4; i++){
      //If no valid texture at this pixel don't blend it
      if(VaryingTexCoord[i].z < 0.0 || VaryingTexCoord[i].x < 0.0 || VaryingTexCoord[i].x > 1.0 ||VaryingTexCoord[i].y <0.0||VaryingTexCoord[i].y >1.0)
            continue;
      validPix=true;

      float r=VaryingTexCoord[i].z;
      float W=exp(-r*10.0*16.0*Cb[j]);


      if(j == 0)
        outComp[j]+=((texture2D(theTexture,VaryingTexCoord[i].xy)-texture2DLod(theTexture,VaryingTexCoord[i].xy,mipmapL[1]))*W);
      else if (j==1)
        outComp[j]+=((texture2DLod(theTexture,VaryingTexCoord[i].xy,mipmapL[1])-texture2DLod(theTexture,VaryingTexCoord[i].xy,mipmapL[2]))*W);
      else if(j==2)
        outComp[j]+=(texture2DLod(theTexture,VaryingTexCoord[i].xy,mipmapL[2])*W);

      WSum[j]+=W;
    }
  }
  //No valid texture at all put color
  if(!validPix)
    return gl_Color;
  outComp[0]=outComp[0]/WSum[0];
  outComp[1]=outComp[1]/WSum[1];
  outComp[2]=outComp[2]/WSum[2];

  outP = outComp[0]+outComp[1]+outComp[2];
  return outP;

}


void main()
{
  vec4 color;
  vec3 usedWeights;
  usedWeights=weights;


  if(usedWeights.x==0.0 && usedWeights.y==0.0 &&usedWeights.z==0.0)
    usedWeights=vec3(0.710000,0.650000,0.070000);

 if(shaderOut == 0)
    color=texture2D(theTexture,VaryingTexCoord[0].xy);
  else if(shaderOut ==1)
    color=texture2D(theTexture,VaryingTexCoord[1].xy);
  else if(shaderOut ==3)
       color=texture2D(theTexture,VaryingTexCoord[2].xy);
    else
       color=texture2D(theTexture,VaryingTexCoord[3].xy);

  gl_FragColor = color;
}


