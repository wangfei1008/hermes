#include "component_loader.h"
#include "log/log.h"

ComponentLoader::ComponentLoader(const std::string& libname)
	:m_plib(NULL)
{
	m_plib = load(libname);
	if (m_plib != NULL)
	{
		m_create_func = (CreateComponent)m_plib->resolve(LIB_FUN_CREATE);
		m_release_func = (ReleaseComponent)m_plib->resolve(LIB_FUN_RELEASE);
	}
}

IComponent* ComponentLoader::create()
{
	IComponent* p = NULL;
	if (m_plib != NULL)
	{
		p = init_component();
		m_list.push_back(p);
	}
	return p;
}

bool ComponentLoader::release(IComponent* pcomponent)
{
	if (m_plib != NULL)
	{
		for (auto it = m_list.begin(); it != m_list.end(); it++)
		{
			if (*it == pcomponent)
			{
				release_component(pcomponent);
				m_list.erase(it);
				break;
			}
		}

		if (m_list.size() == 0)
		{
			m_plib->unload();
			delete m_plib;
			m_plib = NULL;
			return true;
		}
	}

	return false;
}

bool ComponentLoader::release()
{
	if (m_plib != NULL)
	{
		for (auto it = m_list.begin(); it != m_list.end(); it++)
			release_component(*it);
		m_list.clear();

		m_plib->unload();
		delete m_plib;
		m_plib = NULL;
	}

	return true;
}

LibraryLoader* ComponentLoader::load(const std::string& libname)
{
	LibraryLoader* qLib = NULL;
	if (!libname.empty())
	{
		qLib = new LibraryLoader(libname);
		if (!qLib->load())
		{
			LOGERROR("[Library]%s Dynamic link library loaded fail", libname.c_str());
		}
		else
			LOGINFO("[Library]%s dynamic link library loaded successfully", libname.c_str());
	}
	else
	{
		LOGERROR("[Library]%s unsupported platform",libname.c_str());
	}
	return qLib;
}

IComponent* ComponentLoader::init_component()
{
	IComponent* p = NULL;
	if (m_create_func) {
		LOGINFO("[Library]%s create_lib function parsing successful", m_plib->file_name().c_str());
		m_create_func(&p);
	}
	else {
		LOGERROR("[Library]%s create_lib function parsing fail", m_plib->file_name().c_str());
	}

	return p;
}

bool ComponentLoader::release_component(IComponent* pcomponent)
{
	if (m_release_func) {
		LOGINFO("[Library]%s release_lib function parsing successful", m_plib->file_name().c_str());
		return m_release_func(&pcomponent);

	}
	else {
		LOGERROR("[Library]%s release_lib function parsing fail", m_plib->file_name().c_str());
	}
	return false;
}
