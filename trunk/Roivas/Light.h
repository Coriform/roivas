#pragma once

#include "Component.h"

namespace Roivas
{
	class Light : public Component
	{
		public:
			Light();
			Light(const Light& l);
			~Light();
			Light* Clone();
			void Initialize();
			void Deserialize(FileIO& fio, Json::Value& root);

		// Data
			vec3 Color;
			float Radius;
	};
}