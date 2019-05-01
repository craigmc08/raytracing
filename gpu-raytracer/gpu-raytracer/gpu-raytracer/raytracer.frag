in vec2 position;

uniform vec3 startPos;
uniform vec3 forward;
uniform vec3 up;
uniform vec3 right;

vec3 GetCameraRay(vec2 pos)
{
	vec3 ray = right * pos.x + up * pos.y + forward;
	return normalize(ray);
}

float BoxTest(vec3 p, vec3 c1, vec3 c2)
{
	c1 = p - c1;
	c2 = c2 - p;
	return -min(
		min(
			min(c1.x, c2.x),
			min(c1.y, c2.y)
		),
		min(c1.z, c2.z)
	);
}
float SphereTest(vec3 p, vec3 c, float r)
{
	vec3 d = c - p;
	return length(d) - r;
}
float Query(vec3 position, inout int hitType)
{
	float dist = 1e9;
    hitType = 0;

    dist = SphereTest(position, vec3(0, 1, 0), 1.);
	hitType = 2;

	float floorDist = position.y;
	if (floorDist < dist)
	{
		dist = floorDist;
		hitType = 1;
	}

	return dist;
}
int RayMarch(vec3 origin, vec3 dir, inout float traveled, inout vec3 normal)
{
	float dist = 0.;
	int noHitCount = 0;
	for (float total_d = 0.; total_d < 100.; total_d += dist)
	{
		vec3 hitPoint = origin + dir * total_d;
		int hitType;
		dist = Query(hitPoint, hitType);
		if (dist < 0.01)
		{
			traveled = total_d;
			normal = normalize(vec3(
				Query(hitPoint + vec3(0.01, 0, 0), noHitCount) - dist,
				Query(hitPoint + vec3(0, 0.01, 0), noHitCount) - dist,
				Query(hitPoint + vec3(0, 0, 0.01), noHitCount) - dist
			));
			return hitType;
		} else if (++noHitCount > 399)
		{
			traveled = total_d;
			normal = vec3(0.);
			return 0;
		}
	}

	traveled = 0.;
	normal = vec3(0.);
	return 0;
}

vec3 CheckerColor(vec3 pos)
{
	float spacing = 1.;
	float quarterSpacing = spacing / 4.;
	float cz = mod(abs(pos.z) + quarterSpacing, spacing) / spacing * 2. - 1.;
	float cx = mod(abs(pos.x) + quarterSpacing, spacing) / spacing * 2. - 1.;
	return cz*cx < 0. ? vec3(0.7) : vec3(1.);
}
vec3 GetReflectance(int hitType)
{
	if (hitType == 2) return vec3(1, 0, 0);
	else if (hitType == 3) return vec3(0.8, 0.7, 1);
	else if (hitType == 7) return vec3(1, 0.7, 0.9);
	else if (hitType == 5) return vec3(0.7, 0.9, 1);
	else if (hitType == 6) return vec3(1, 0.75, 0.7);
	else if (hitType == 8) return vec3(0.7, 0.75, 1);
	else if (hitType == 4) return vec3(0.7, 1, 0.75);
	else return vec3(1);
}

vec3 TracePath(vec3 origin, vec3 dir)
{
	vec3 skyColor = vec3(0.6, 0.8, 1);
	vec3 lightDir = normalize(vec3(-0.2, 0.3, -0.5));

	vec3 col;
	vec3 multiplier = vec3(1.);

	vec3 o = origin;
	vec3 d = dir;

	for (int depth = 0; depth < 12; depth++)
	{
		float traveled;
		vec3 normal;
		int hitType = RayMarch(o, d, traveled, normal);
		vec3 samplePos = o + d * traveled;

		if (hitType == 0)
		{
			col += skyColor * multiplier;
			break;
		}

		float sunIncidence = dot(normal, lightDir);
		vec3 incomingLight;
		if (sunIncidence > 0.) {
			float tsun; vec3 nsun;
			if (RayMarch(samplePos + normal * 0.05, lightDir, tsun, nsun) == 0)
			{
				incomingLight = vec3(1, 1, 1) * sunIncidence;
			}
		}

		if (hitType == 2 || hitType == 3 || hitType == 4 || hitType == 5 || hitType == 6 || hitType == 7 || hitType == 8)
		{
			o = samplePos + normal * 0.05;
			d = d + normal * (dot(normal, dir) * -2.);

			multiplier = multiplier * GetReflectance(hitType);
		}
		else if (hitType == 1)
		{
			col += CheckerColor(samplePos) * incomingLight * multiplier;
			return col;
		}
	}

	return col;
}

void main()
{
	vec3 ray = GetCameraRay(position);

	vec3 col = TracePath(startPos, ray);

	gl_FragColor = vec4(col.rgb, 1);
}