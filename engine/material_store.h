#pragma once
#include <engine/shader.h>

#include <map>

namespace phoenix
{
	class MaterialStore
	{
	public:
		static MaterialStore& getInstance();

		Shader* getMaterial(const std::string);

		~MaterialStore();

	private:
		std::map<std::string, Shader*> _materials;

		MaterialStore();
		MaterialStore(MaterialStore const&) = delete;
		void operator=(MaterialStore const&) = delete;
	};
}