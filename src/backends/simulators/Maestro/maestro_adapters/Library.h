/**
 * @file Library.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * The library class.
 *
 * Used to dynamically load a library on linux or windows.
 */

#pragma once

#ifndef _LIBRARY_H
#define _LIBRARY_H

#include <iostream>

#ifdef __linux__

#include <dlfcn.h>

#elif defined(_WIN32)

#include <windows.h>

#endif 

namespace Utils {

	class Library
	{
	public:
		Library(const Library&) = delete;
		Library& operator=(const Library&) = delete;
		Library(Library&&) = default;
		Library& operator=(Library&&) = default;

		Library() noexcept
		{
		}

		virtual ~Library()
		{
			if (handle)
#ifdef __linux__
				dlclose(handle);
#elif defined(_WIN32)
				FreeLibrary(handle);
#endif
		}

		virtual bool Init(const char* libName) noexcept
		{
#ifdef __linux__
			handle = dlopen(libName, RTLD_NOW);

			if (handle == nullptr)
			{
				const char* dlsym_error = dlerror();
				if (dlsym_error)
					std::cout << "Library: Unable to load library, error: " << dlsym_error << std::endl;

				return false;
			}
#elif defined(_WIN32)
			handle = LoadLibraryA(libName);
			if (handle == nullptr)
			{
				const DWORD error = GetLastError();
				std::cout << "Library: Unable to load library, error code: " << error << std::endl;
				return false;
			}
#endif

			return true;
		}

		void* GetFunction(const char* funcName) noexcept
		{
#ifdef __linux__
			return dlsym(handle, funcName);
#elif defined(_WIN32)
			return GetProcAddress(handle, funcName);
#endif
		}

		const void* GetHandle() const noexcept
		{
			return handle;
		}

	private:
#ifdef __linux__
		void* handle = nullptr;
#elif defined(_WIN32)
		HINSTANCE handle = nullptr;
#endif
	};

}

#endif
