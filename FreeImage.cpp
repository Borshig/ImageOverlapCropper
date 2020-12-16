// FreeImage.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include "include/tbb/parallel_for.h"
#include "dirent.h"
#include "include/FreeImage.h"



using UINT = unsigned int;

UINT fileNameLength(const wchar_t* string) noexcept
{
	UINT value = 0;
	if (string)
	{
		for (UINT i = 0; string[i] != '\0'; i++)
		{
			value = i;
		}
	}
	return value;
}

wchar_t* findLastSymbol(wchar_t* string, const wchar_t symbol) noexcept
{
	wchar_t* pointer = nullptr;
	if (string)
	{
		for (UINT i = 0; string[i] != '\0'; i++)
		{
			if (string[i] == symbol)
				pointer = &string[i];

		}
	}
	return pointer;
}


wchar_t* findSymbol(wchar_t* string, const wchar_t symbol, UINT pisition = 1 ) noexcept
{
	wchar_t* pointer = nullptr;
	if (string)
	{
		for (UINT i = 0, p = 0; string[i] != '\0' && p != pisition; i++)
		{
			if (string[i] == symbol)
			{
				pointer = &string[i];
				p++;
			}
		}
	}
	
	return pointer;
}

FREE_IMAGE_FORMAT getFIFfrom(const wchar_t* ext) noexcept
{
	if (ext[1] == L'p' && ext[2] == L'n' && ext[3] == L'g') return FIF_PNG;
	if (ext[1] == L'j' && ext[2] == L'p' && (ext[3] == L'g' || (ext[3] == L'e' && ext[4] == L'g'))) return FIF_JPEG;
	return FIF_UNKNOWN;
}

//hardcoded
bool inList(const wchar_t* ext) noexcept
{
	if (ext)
	{
		if (ext[1] == L'p' && ext[2] == L'n' && ext[3] == L'g') return true;
		if (ext[1] == L'j' && ext[2] == L'p' && (ext[3] == L'g' || (ext[3] == L'e' && ext[4] == L'g'))) return true;
	}
	
	return false;
}

wchar_t* chtowch(const char* str) noexcept
{
	UINT strlenght = 0;
	for (UINT i = 0; str[i] != '\0'; i++)
	{
		strlenght++;		
	}
	wchar_t* wstr = new wchar_t[strlenght * sizeof(wchar_t)];
	for (UINT i = 0; str[i] != L'\0'; i++)
	{
		wstr[i] = (wchar_t)str[i];
	}
	wstr[strlenght] = L'\0';

	return wstr;
}


inline void presentHelp() noexcept
{
	std::cout << "if (argv[i][0] == '-' || argv[i][0] == '/')" << std::endl
	<<"{" << std::endl
	<<"	if (strcmp(argv[i], \"-help\") == NULL || strcmp(argv[i], \"/help\") == NULL) { presentHelp();  return -1; }" << std::endl
	<<"	if (strcmp(argv[i], \"-w\") == NULL || strcmp(argv[i], \"/w\") == NULL) workMode = atoi(argv[i + 1]);" << std::endl
	<<"	if (strcmp(argv[i], \"-path\") == NULL || strcmp(argv[i], \"/path\") == NULL) workDir = (argv[i + 1]);" << std::endl
	<<"	if (strcmp(argv[i], \"-h\") == NULL || strcmp(argv[i], \"/h\") == NULL) cHeight = atoi(argv[i + 1]);" << std::endl
	<<"	if (strcmp(argv[i], \"-m\") == NULL || strcmp(argv[i], \"/m\") == NULL) mSize = atoi(argv[i + 1]);" << std::endl
	<<"	if (strcmp(argv[i], \"-o\") == NULL || strcmp(argv[i], \"/o\") == NULL) overlap = atoi(argv[i + 1]);" << std::endl
	<<"	if (strcmp(argv[i], \"-i\") == NULL || strcmp(argv[i], \"/i\") == NULL) inif = (argv[i + 1]);" << std::endl
	<<"	if (strcmp(argv[i], \"-e\") == NULL || strcmp(argv[i], \"/e\") == NULL) outputExt = (argv[i + 1])" << std::endl
	<<"	if (strcmp(argv[i], \"-x\") == NULL || strcmp(argv[i], \"/x\") == NULL) outputMultiply = atoi(argv[i + 1]);" << std::endl
	<<"}" << std::endl;
	getchar();
}

int crop(UINT overlap, UINT cHeight, UINT mSize, UINT outputMultiply, const wchar_t* workdir, const wchar_t* inifile)
{

	//Using UNICODE;  ANCII is fucking trash
	_WDIR* dir;
	struct  _wdirent* ent;
	std::vector<std::wstring> files;
	std::vector<std::wstring> exten;
	std::wofstream stream;

	std::wstring wini = L"";
	wini += workdir;
	wini += L"\\";
	wini += inifile;

	// Проходимся по списку всех фаилов и добавляем их в список	// hardcoded
	if ((dir = _wopendir(workdir)) != nullptr)
	{
		while ((ent = _wreaddir(dir)) != nullptr)
		{
			// pushback if file have whitelisted (is picture) extension
			if (fileNameLength(ent->d_name) > 3) //exclude "." and ".." in  results
			{
				bool inPicList;
				wchar_t* ext = findLastSymbol(ent->d_name, '.');
				if(ext)
					inPicList = inList(ext);
				else
					break;

				if (inPicList)
				{
					files.emplace_back(ent->d_name); // pushback
					exten.emplace_back(ext);
				}
			}
		}
		_wclosedir(dir);
	}
	else
	{
		/* could not open directory */
		perror("");
		return EXIT_FAILURE;
	}



	stream.open(wini.c_str());
	dir = nullptr;

	std::vector<FIBITMAP*> dib;
	
	UINT count = files.size();
	if (count < 1)
		return 0x01;

	for (UINT i = 0; i < count; i++)
	{
		//LOAD
		std::wstring path = L"";
		path += workdir;
		path += L"\\";
		path += files[i].c_str();
		FREE_IMAGE_FORMAT fif = getFIFfrom(exten[i].c_str());
		dib.emplace_back(FreeImage_LoadU(fif, path.c_str())); // load image with fif format			
		//LOAD complete

		const UINT width = FreeImage_GetWidth(dib[i]);
		const UINT height = FreeImage_GetHeight(dib[i]);
		//UINT bytebpp = FreeImage_GetLine(dib[i]) / FreeImage_GetWidth(dib[i]);

		//crop if to many pixels or crop in args		
		if (width * height > mSize)
		{
			cHeight = (static_cast<UINT>(mSize / width) - overlap);

			UINT save_counter = 0;


			wchar_t* p = (wchar_t*)files[i].c_str();
			for (UINT i = 0; p[i] != '\0'; i++)
			{
				stream << p[i];
			}

			stream << ":" << width * outputMultiply << ":" << height * outputMultiply << ":" << overlap * outputMultiply << std::endl;

			for (UINT cDown = 0; cDown + cHeight <= height + cHeight; cDown += cHeight)
			{
				UINT fibHeight = 0;
				if (cHeight + overlap < height - cDown)
				{
					fibHeight = cHeight + overlap;
				}
				else
				{
					fibHeight = height - cDown;
				}
				if (fibHeight <= overlap) return 0xFE;
				FIBITMAP* fib = FreeImage_Allocate(width, fibHeight, 24);

				for (UINT y = 0; y < fibHeight + cDown; y++)
				{
					for (UINT x = 0; x < width; x++)
					{
						// Set pixel color to green with a transparency of 128
						RGBQUAD color[] = { 0,0,0,0 };
						FreeImage_GetPixelColor(dib[i], x, y + cDown, color);
						FreeImage_SetPixelColor(fib, x, y, color);
					}
				}

				std::wstring spath = L"";
				spath += workdir;								// add folder 
				spath += L"\\";									// add '\'
				for (UINT j = 1; j < 5; j++)
				{
					if (save_counter < (pow(10,j)) )spath += std::to_wstring(0);
				}
				spath += std::to_wstring(save_counter);			// add _number
				spath += L"_";									// add '_'
														// delete ext from filename
				spath += files[i].c_str();				// add filename with ext	
				stream << save_counter;
				stream << "_";
				wchar_t *p = (wchar_t*)files[i].c_str();
				for (UINT j = 0; p[j] != '\0'; j++)
				{
					stream << (char)p[j];
				}

				stream << std::endl;
				
				//save as png 
				FreeImage_SaveU(FIF_PNG, fib, spath.c_str());   // save
				FreeImage_Unload(fib);							// and release
				save_counter++;

			}
		}
		// else pictures already fine)
	

		FreeImage_Unload(dib[i]); //RELEASE
	}
	stream.close();
	return 0;
}
 
int join(std::wstring outputfilename, const wchar_t* workdir, const wchar_t* inifile,const wchar_t* outputExt)
{
	
	//Using UTF-16;  ANCII is fucking trash // yeap, but char can contain UTF-8
	_WDIR* dir;
	struct  _wdirent* ent;
	std::vector<std::wstring> files;
	std::vector<std::wstring> exten;
	
	std::wifstream stream;
	
	std::wstring wini = L"";
	wini += workdir;
	wini += L"\\";
	wini += inifile;

	bool bIniFileNotExist = true;
	


	// Проходимся по списку всех фаилов и добавляем их в список	// hardcoded
	if ((dir = _wopendir(workdir)) != nullptr)
	{
		while ((ent = _wreaddir(dir)) != nullptr)
		{
			// pushback if file have whitelisted (is picture) extension
			if (fileNameLength(ent->d_name) > 3) //exclude "." and ".." in  results
			{
				wchar_t* ext = findLastSymbol(ent->d_name, '.');

				if (!ext)
				{
					std::cout << "Can't find file extenthion " << std::endl;					
				}
				
				std::wcout << L"found file: " << ent->d_name << std::endl;
				

				
				for (UINT i = 0; ent->d_name[i] != '\0' && inifile[i] != '\0'; i++)
				{
					
						

					if (ent->d_name[i] != inifile[i]) // скипает фаилы которые не должны попасть в список?
					{
						break;
					}
				}

				bIniFileNotExist = false;
				
				if (inList(ext))
				{
					files.emplace_back(ent->d_name); // pushback
					exten.emplace_back(ext);
					
					
				}
			}
		}
		_wclosedir(dir);
	}
	else
	{
		/* could not open directory */
		perror("");
		return EXIT_FAILURE;
	}


	if (bIniFileNotExist)
	{
		std::cout << "ini file not exist in this folder" << std::endl;
		return -13;
	}

	stream.open(wini.c_str());

	dir = nullptr;

	std::vector<FIBITMAP*> dib;

	const UINT count = files.size();
	if (count < 1)
		return 0x01; 

	std::wstring st = L"";
	stream >> st;
	


	wchar_t* intState = nullptr;
	wchar_t* token = nullptr;
	UINT width = 0, height = 0 , overlaps = 0;
	try 
	{
		token = wcstok((wchar_t*)st.c_str(), L":", &intState);
		token = wcstok(nullptr, L":", &intState);
		width = _wtoi(token);
		token = wcstok(nullptr, L":", &intState);
		height = _wtoi(token);
		token = wcstok(nullptr, L":", &intState);
		overlaps = _wtoi(token);
	}
	catch (...)
	{
		std::cout << "app::join(args) throw error failed in try-catch(token). May be you lost ini file?" << std::endl;
	}
	
	FIBITMAP* fib = FreeImage_Allocate(width, height, 24);
	
	std::wstring opath = L"";

	UINT offsetY = 0;


	
	for (UINT i = 0; i < count; i++)
	{
		//if(!(wcsstr(files[i], sourceFile))) break;

		//LOAD
		std::wstring path = L"";
		path += workdir;
		path += L"\\";
		opath = path;
		path += files[i].c_str();
	
		FREE_IMAGE_FORMAT fif = getFIFfrom(exten[i].c_str());

		if (fif == FIF_UNKNOWN)
		{
			std::cout << "app::FreeImage::getFIFfrom cant get file format" << std::endl;
			return -9;
		}

		dib.emplace_back(FreeImage_LoadU(fif, path.c_str())); // load image with fif format			
		//LOAD complete

		if (!dib[i])
		{
			std::cout << "file wasn't load by app::FreeImage::FreeImage_LoadU(dib[i]);" << std::endl;
			std::cout << "this may be due to the fact that the file has the wrong extension" << std::endl;
			return -11;
		}

	
		const UINT fWidth = FreeImage_GetWidth(dib[i]);
		const UINT fHeight = FreeImage_GetHeight(dib[i]);

		

		if (fWidth == 0 || fHeight == 0)
		{

			std::cout << "app::FreeImage::FreeImage_LoadU cant get file width and height" << std::endl;
			return -12;
		}


		//UINT bytebpp = FreeImage_GetLine(dib[i]) / FreeImage_GetWidth(dib[i]);
		
		UINT y = 0;
	
		if (offsetY == 0)
		{
			for (y = offsetY; y < fHeight + offsetY - overlaps; y++)
			{
				for (UINT x = 0; x < width; x++)
				{
					// Set pixel color to green with a transparency of 128
					RGBQUAD color[] = { 0,128,0,0 };
					FreeImage_GetPixelColor(dib[i], x, y, color);
					FreeImage_SetPixelColor(fib, x, y, color);
				}
			}
		}
		else
		{
			for (y = offsetY; y < fHeight + offsetY; y++)
			{
				for (UINT x = 0; x < width; x++)
				{
					// Set pixel color to green with a transparency of 128
					RGBQUAD color[] = { 0,128,0,0 };
					FreeImage_GetPixelColor(dib[i], x, y - offsetY, color);
					FreeImage_SetPixelColor(fib, x, y, color);
				}
			}
	
		}
		if (offsetY == 0)
		{
			offsetY = y;
		}
		else
		{
			offsetY = y - overlaps;
		}
	
	
		FreeImage_Unload(dib[i]); //RELEASE
	}
	
	

	opath += outputfilename;

	FreeImage_SaveU(FIF_JPEG, fib, opath.c_str(), JPEG_QUALITYSUPERB);
	//FreeImage_SaveU(FIF_PNG, fib, opath.c_str());
	
		



	FreeImage_Unload(fib);

	stream.close();

	
	return 0x00;
}

int main(int argc,char* argv[]  )
{
	//bool bCropImage = false;
	UINT cHeight = 0;
	UINT overlap = 25;
	UINT workMode = 0;
	UINT mSize = 777000;

	char* outputExt = nullptr;
	char* workDir = nullptr;
	char* inif = nullptr;
	UINT outputMultiply = 1;

	// берем параметры из аргументов

	if (argc == 0)
	{
		presentHelp();
		return -1;
	}


	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' || argv[i][0] == '/')
		{			
			if (strcmp(argv[i], "-help") == NULL || strcmp(argv[i], "/help")  == NULL) { presentHelp();  return -1; }
			if (strcmp(argv[i], "-w")	 == NULL || strcmp(argv[i], "/w")	  == NULL) workMode			= atoi(argv[i + 1]);
			if (strcmp(argv[i], "-path") == NULL || strcmp(argv[i], "/path")  == NULL) workDir			=     (argv[i + 1]);			
			if (strcmp(argv[i], "-h")	 == NULL || strcmp(argv[i], "/h")	  == NULL) cHeight			= atoi(argv[i + 1]);	
			if (strcmp(argv[i], "-m")	 == NULL || strcmp(argv[i], "/m")	  == NULL) mSize			= atoi(argv[i + 1]);
			if (strcmp(argv[i], "-o")	 == NULL || strcmp(argv[i], "/o")	  == NULL) overlap			= atoi(argv[i + 1]);
			if (strcmp(argv[i], "-i")	 == NULL || strcmp(argv[i], "/i")	  == NULL) inif				=	  (argv[i + 1]);
			if (strcmp(argv[i], "-e")	 == NULL || strcmp(argv[i], "/e")	  == NULL) outputExt		=	  (argv[i + 1]);
			if (strcmp(argv[i], "-x")	 == NULL || strcmp(argv[i], "/x")	  == NULL) outputMultiply	= atoi(argv[i + 1]);
			//if (strcmp(argv[i], "-c")	 == NULL || strcmp(argv[i], "/c")	  == NULL) bCropImage		= true;
		}
	}
	   
	if (!workDir)
	{
		presentHelp();
		return -2;
	}
	const wchar_t* wdir = chtowch((const char*)workDir);
	const wchar_t* inif_w = nullptr;
	if (inif)
	{
		inif_w = chtowch((const char*)inif);
	}		
	else
	{
		std::cout << "ini file wasn't set" << std::endl;
		if (wdir) delete[] wdir;		
		return -3;
	}
	

	

	if (workMode == 1)
	{
		auto err = crop(overlap, cHeight, mSize, outputMultiply, wdir, inif_w);
		if(err)
		{
			std::cout << "app::crop(args) return error: " << err << std::endl;
			if (wdir) delete[] wdir;
			if (inif_w)	delete[] inif_w;
			return -4;
		}
	}
	else if (workMode == 2)
	{

		std::wstring ofile_name;
		std::wifstream wifstream;
	
		std::wstring wini = L"";
		wini += wdir;
		wini += L"\\";
		wini += inif_w;

		wifstream.open(wini);
		wifstream >> ofile_name;

		/*if(ext)*/


		for (UINT i = 0; ;i++)
		{			
			if (ofile_name.c_str()[i] == L'\0' || ofile_name.c_str()[i] == L':')
			{
				ofile_name[i] = L'\0';
				break;
			}			
		}
		wifstream.close();

		auto err = join(ofile_name.c_str(), wdir, inif_w, chtowch(outputExt) );
		if (err)
		{
			std::cout << "app::join(args) return error: " << err << std::endl;
			if (wdir) delete[] wdir;
			if (inif_w)	delete[] inif_w;
			return -5;
		}
	}
	else
	{
		if (wdir) delete[] wdir;
		if (inif_w)	delete[] inif_w;
		presentHelp();
		return -6;
	}	
	if (wdir) delete[] wdir;
	if (inif_w)	delete[] inif_w;
	return 0;
}