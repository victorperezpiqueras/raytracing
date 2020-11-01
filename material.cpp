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
	bool isTrans;
	///a
	glm::vec3 N = shadInfo.normal;
	
	V = -shadInfo.rayDir;
	VdotN = glm::dot(V, shadInfo.normal);
	isTrans = (Kt != glm::vec3(0.0, 0.0, 0.0));
	if (VdotN < 0) {

		// The viewer stares at an interior or back face of the object,
		// we will only illuminate it if material is transparent
		if (isTrans) {
			shadInfo.normal = -shadInfo.normal;  // Reverse normal
			VdotN = -VdotN;
			ratio = 1.0 / ior; // Ray always comes from vacuum (hollow objects)
			//ratio = ior;  // Use this instead for solid objects
		}
		else
			return color;
	}
	else {

		// The viewer stares at a front face of the object
		if (isTrans)
			ratio = 1.0 / ior; // Ray comes from vacuum
	}

	// To do ...

	//LOCAL:
	// ------Reflexion luz ambiental:
	// Ia=Ka�Ila->coeficiente reflexion luz ambiental
	// ------Reflexion difusa:
	// Id=
	//if dot(L,N)>0 -> Kd�Ilid�dot(L,N)
	// else 0
	// ------Reflexion especular:
	// Is=
	//if dot(L,N)>0 ->Ks�Ils�dot(R,V)^n
	// else 0
	//Ks->coeficiente reflexion especular
	//n->exponente reflexion especular
	//R->2(L�N)N-L
	//GLOBAL:
	//I=Ilocal+Kr�Ir+Kt�It
	//Kr->Coeficiente reflexion (espec) global
	//Kt->Coeficiente transmision (espec) global
	//Ilocal->
	//Idt=
	//if dot(L,N)<0 ->Kdt�Ild �(dot(L,-N))
	// else 0
	//Ist=
	//if dot(L,N)<0 ->Kst�Ils �(dot(T,V))^n
	// else 0

	//CODIGO:
	glm::vec3 Ilocal = glm::vec3(0.0, 0.0, 0.0);

	glm::vec3 Ia = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 Id = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 Is = glm::vec3(0.0, 0.0, 0.0);

	glm::vec3 Idt = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 Ist = glm::vec3(0.0, 0.0, 0.0);

	glm::vec3 Ir = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 It = glm::vec3(0.0, 0.0, 0.0);

	glm::vec3 R, L, Vobs, T;
	float LdotN;

	Light* light = shadInfo.pWorld->lights.First();
	while (light != NULL) {
	/* Iluminaci�n local */
		// para cada punto de luz i->calcular su Li�N
		L = glm::normalize(light->position - shadInfo.point);
		//L = V;
		LdotN = glm::dot(L, N);
		//reflexion ambiental
		Ia += Ka * light->Ia;

		//(especular)
		R = 2 * LdotN * N - L;
		//AB= B-A
		Vobs = glm::normalize(shadInfo.pWorld->eye - shadInfo.point);

		if (LdotN > 0) {
			//reflexion difusa
			Id += Kd * light->Id * LdotN;

			//reflexion especular
			Is += Ks * light->Is * pow((glm::dot(R, Vobs)), n);
		}

		//transmision--------------------------------------------------------------
		if (LdotN < 0) {   //al haber cambiado el signo de la normal al principio hay que usar el V�N//if (VdotN < 0) {
			//transmision difusa
			Idt += Kdt * light->Id * glm::dot(L, -N);//ya se cambia el signo a la normal
			//Idt += Kdt * light->Id * glm::dot(L, shadInfo.normal);

			//transmision especular
			float b, cos, rad;
			//coseno del angulo con vectores normalizados==dot->L y N
			cos = glm::dot(-L, -N);//porque ya se hace al principio del todo
			//cos = glm::dot(L, shadInfo.normal);
			//comprobar que la raiz es positiva
			rad = 1 + pow(ratio, 2) * (pow(cos, 2) - 1);
			if (rad >= 0) {
				b = ratio * cos - sqrt(rad);//todo
				T = glm::normalize((ratio * -L) + b * (-N));//todo
				Ist += Kst * light->Is * pow((glm::dot(T, Vobs)), n);//todo
			}
		}
		//ning�n otro cuerpo se interpone entre la luz y la superficie??? T4-d59
		Ilocal += Ia + Id + Is /*+ Idt + Ist*/;
		
		light = shadInfo.pWorld->lights.Next();
	}
	color += Ilocal;

	/* Iluminaci�n global */
	//todo saber si la N de L�N en transmision es la misma que la del rayo principal o al salir es otra N
	if (shadInfo.depth < shadInfo.pWorld->maxDepth) {
		if (!isTrans) {
			color += Kr * shadInfo.pWorld->Trace(shadInfo.point, R, shadInfo.depth + 1);
		}
		if (isTrans) {
			//color += Kt * shadInfo.pWorld->Trace(shadInfo.point, T, shadInfo.depth + 1);
		}//TODO funciona a medias el transparente-> le falta reflejar el rayo bien
		// la T va a haber que cambiarla porque los rayos son distintos segun la practica
	}

	return color;
}
