/* Material.cpp
*
* Realidad Virtual y Aumentada.
*
* Practice 2.
* Ray tracing.
*
* Jose Pascual Molina Masso.
* Escuela Superior de Ingenieria Informatica de Albacete.
*/


#include "glm/glm.hpp" // glm::vec3, glm::dot

#include "Material.h"
#include "light.h"
#include "lightlist.h"
#include "world.h"
#include "limits.h"


/* Constructors */
Material::Material(const glm::vec3& diff) {

	Ka = glm::vec3(0.0, 0.0, 0.0);
	Kd = diff;
	Kdt = glm::vec3(0.0, 0.0, 0.0);
	Ks = glm::vec3(0.0, 0.0, 0.0);
	Kst = glm::vec3(0.0, 0.0, 0.0);
	n = 0;
	Ie = glm::vec3(0.0, 0.0, 0.0);
	Kr = glm::vec3(0.0, 0.0, 0.0);
	Kt = glm::vec3(0.0, 0.0, 0.0);
	ior = 0.0;
}

Material::Material(const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& diffTrans,
	const glm::vec3& spec, const glm::vec3& specTrans, int shine, const glm::vec3& emis,
	const glm::vec3& refl, const glm::vec3& trans, float index) {

	Ka = amb;
	Kd = diff;
	Kdt = diffTrans;
	Ks = spec;
	Kst = specTrans;
	n = shine;
	Ie = emis;
	Kr = refl;
	Kt = trans;
	ior = index;
}

/* Implements the global illumination model */
glm::vec3 Material::Shade(ShadingInfo& shadInfo)
{
	glm::vec3 color(0.0, 0.0, 0.0), V;
	float VdotN, ratio = 0.0;
	bool isTrans, isSpec;
	///a
	//glm::vec3 N = glm::normalize(shadInfo.normal);

	V = -shadInfo.rayDir;
	VdotN = glm::dot(V, shadInfo.normal);
	isTrans = (Kt != glm::vec3(0.0, 0.0, 0.0));
	isSpec = (Ks != glm::vec3(0.0, 0.0, 0.0)) || (Kst != glm::vec3(0.0, 0.0, 0.0));
	if (VdotN < 0) {

		// The viewer stares at an interior or back face of the object,
		// we will only illuminate it if material is transparent
		if (isTrans) {
			shadInfo.normal = -shadInfo.normal;  // Reverse normal
			VdotN = -VdotN;
			ratio = 1.0 / ior; // Ray always comes from vacuum (hollow objects)
			//ratio = ior;  // Use this instead for solid objects
		}
		else {
			return color;
		}
	}
	else {

		// The viewer stares at a front face of the object
		if (isTrans)
			ratio = 1.0 / ior; // Ray comes from vacuum
	}

	// To do ...

	//VARIABLES DE ILUMINACION
	glm::vec3 Ia, Id, Is, Idt, Ist, Ir, It, Ilocal = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 L, R, T;
	float LdotN;

	Light* light = shadInfo.pWorld->lights.First();
	while (light != NULL) {
		//resetear variables
		Ia = glm::vec3(0.0, 0.0, 0.0);
		Id = glm::vec3(0.0, 0.0, 0.0);
		Is = glm::vec3(0.0, 0.0, 0.0);
		Idt = glm::vec3(0.0, 0.0, 0.0);
		Ist = glm::vec3(0.0, 0.0, 0.0);

		//vector L de sombra
		L = light->position - shadInfo.point;
		L = glm::normalize(L);
		LdotN = glm::dot(L, shadInfo.normal);
		shadInfo.pWorld->numShadRays++;

		//obtener reduccion de intensidad por objetos que tapan la luz
		glm::vec3 lightReduction = glm::vec3(1.0, 1.0, 1.0);
		Object* nearObj = shadInfo.pWorld->objects.First();
		float taux;
		while (nearObj != NULL) {
			taux = nearObj->NearestInt(shadInfo.point, L);
			if (taux > TMIN) {
				lightReduction *= nearObj->pMaterial->Kt;
			}
			nearObj = shadInfo.pWorld->objects.Next();
		}

		////////////* ILUMINACION LOCAL *//////////////////////////////////

		////////////* Reflexion */////////////

		////////////* Reflexion ambiental *//////
		Ia = Ka * light->Ia;

		if (LdotN > 0) {
			////////////* Reflexión difusa *//////
			Id = Kd * light->Id * LdotN;

			////////////* Reflexión especular *//////
			R = 2 * LdotN * shadInfo.normal - L;
			R = glm::normalize(R);
			Is = Ks * light->Is * pow((glm::dot(R, V)), n);
		}

		////////////* Transmision */////////////
		if (LdotN < 0) {
			////////////* Transmision difusa *//////
			//Idt += Kdt * light->Id * LdotN; // es la normal inversa, porque solo la cambia al principio en funcion de la direccion de V, no de L
			Idt = Kdt * light->Id * glm::dot(L, -shadInfo.normal);

			////////////* Transmision especular *//////
			float b = 0.0, cos, rad;
			//coseno del angulo con vectores normalizados==dot->L y N
			cos = LdotN;
			//cos = glm::dot(-L, -N);
			rad = 1 + (ratio * ratio) * ((cos * cos) - 1);
			//comprobar que la raiz es positiva
			if (rad >= 0) {
				b = ratio * cos - sqrt(rad);
			}
			T = (ratio * (L)) + b * (shadInfo.normal);
			//T = (ratio * (-L)) + b * (-N);
			T = glm::normalize(T);
			Ist = Kst * light->Is * pow((glm::dot(T, V)), n);
		}

		Ilocal += Ia + Id + Is + Idt + Ist;

		//aplicar reduccion de intensidad por objetos que tapan la luz
		Ilocal *= lightReduction;

		light = shadInfo.pWorld->lights.Next();
	}
	Ilocal += Ie;

	color += Ilocal;

	////////////* ILUMINACION GLOBAL *//////////////////////////////////
	if (shadInfo.depth < shadInfo.pWorld->maxDepth) {
		////////////* Reflexion */////////////
		if (isSpec) {
			//R utiliza ahora el rayo del punto de vista en vez del rayo de luz i, 
			R = 2 * VdotN * shadInfo.normal - V;
			R = glm::normalize(R);
			Ir = Kr * shadInfo.pWorld->Trace(shadInfo.point, R, shadInfo.depth + 1);
			color += Ir;
			shadInfo.pWorld->numReflRays++;
		}
		////////////* Transmision */////////////
		if (isTrans) {
			float b = 0.0, cos, rad;
			//cos = glm::dot(-V, -N);//ya está hecho para que sea el negativo
			cos = VdotN;

			rad = 1 + (ratio * ratio) * ((cos * cos) - 1);
			if (rad >= 0) {
				b = ratio * cos - sqrt(rad);
			}
			T = (ratio * (-V)) + b * (shadInfo.normal);
			//T = (ratio * (-V)) + b * (-N);
			T = glm::normalize(T);
			It = Kt * shadInfo.pWorld->Trace(shadInfo.point, T, shadInfo.depth + 1);
			color += It;
			shadInfo.pWorld->numRefrRays++;
		}
	}

	return color;
}

