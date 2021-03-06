/** 
 * @file class1/deferred/deferredUtil.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

uniform sampler2DRect   normalMap;
uniform sampler2DRect   depthMap;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec2 getScreenCoordinate(vec2 screenpos)
{
    vec2 sc = screenpos.xy * 2.0;
    if (screen_res.x > 0 && screen_res.y > 0)
    {
       sc /= screen_res;
    }
    return sc - vec2(1.0, 1.0);
}

vec3 decode_normal(vec2 f)
{
    f = f * 2.0 - 1.0;
 
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    vec3 n = vec3( f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ) );
    float t = clamp( -n.z , 0.0, 1.0);
    n.xy += vec2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t); 
    return normalize( n );
}

vec3 getNorm(vec2 screenpos)
{
   vec2 enc = texture2DRect(normalMap, screenpos.xy).xy;
   return decode_normal(enc);
}

vec3 getNormWithEnvIntensity(vec2 screenpos, out float envIntensity)
{
   vec3 enc = texture2DRect(normalMap, screenpos.xy).xyz;
   envIntensity = enc.z;
   return decode_normal(enc.xy);
}

float getDepth(vec2 pos_screen)
{
    float depth = texture2DRect(depthMap, pos_screen).r;
    return depth;
}

vec4 getPosition(vec2 pos_screen)
{
    float depth = getDepth(pos_screen);
    vec2 sc = getScreenCoordinate(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
    vec4 pos = inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}

vec4 getPositionWithDepth(vec2 pos_screen, float depth)
{
    vec2 sc = getScreenCoordinate(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
    vec4 pos = inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}
