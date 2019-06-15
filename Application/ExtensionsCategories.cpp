#include "ExtensionsCategories.h"
#include <cwctype>
#include <algorithm>

const wchar_t* others[] = { L"dwg", L"snt", L"kdbx", L"bak", L"backup", L"tib", L"iso" ,nullptr };
const wchar_t* docs[] = { L"doc", L"docx", L"docb", L"docm", L"pdf", L"djvu", L"odt", nullptr };
const wchar_t* xls[] = { L"xls", L"xlsx", L"csv", nullptr };
const wchar_t* ppt[] = { L"ppt", L"pptx", nullptr };
const wchar_t* outlook[] = { L"pst", L"ost" , L"msg", L"eml", L"vsd", L"vsdx", nullptr };
const wchar_t* sdocs[] = { L"txt", L"rtf", nullptr };
const wchar_t* images[] = { L"jpeg", L"jpg", L"png", L"gif", L"tiff", L"tif", L"bmp", L"raw", L"psd", L"svg", nullptr };
const wchar_t* archive[] = { L"zip", L"rar", L"7z", L"gz", L"tgz", L"tar", L"gzip", nullptr };
const wchar_t* databases[] = { L"db", L"sql", L"sqlitedb", L"sqlite3", nullptr };
const wchar_t* code[] = { L"c", L"h", L"hpp", L"cpp", L"cxx", L"hxx", L"java", L"class", L"php", L"js", L"html", L"sh", L"asp", L"sh", L"jar", L"rb", L"jsp", L"cs", L"vb", L"pl", L"py", L"rst", nullptr };
const wchar_t* music[] = { L"mp3", L"flac", L"alac", L"wav", L"aac", L"ogg", L"wma", nullptr };
const wchar_t* video[] = { L"mp4", L"mkv", L"wmv", L"flv", L"mpg", L"avi", nullptr };

const wchar_t** allCategories[NUM_CATEGORIES_NO_OTHERS] =
{ 
	docs,
	xls,
	ppt,
	outlook,
	sdocs,
	images,
	archive,
	databases,
	code,
	music,
	video
};

std::unordered_map<std::wstring, uint16_t> reverseCategories;

void filleCategories() {
	uint16_t i = 0;
	for (; i < NUM_CATEGORIES_NO_OTHERS; i++) {
		const wchar_t** cat = allCategories[i];
		for (const wchar_t** it = allCategories[i]; *it != nullptr; ++it) {
			reverseCategories[*it] = i;
		}
	}
	for (const wchar_t** it = others; *it != nullptr; ++it) {
		reverseCategories[*it] = i++;
	}
}

uint16_t ExtensionCategory(const wchar_t* Extension) {
	static bool firstTime = true;
	if (firstTime) {
		filleCategories();
		firstTime = false;
	}
	std::wstring wstr(Extension);
	std::transform(
		wstr.begin(), wstr.end(),
		wstr.begin(),
		std::towlower);
	if (reverseCategories.count(wstr))
		return reverseCategories.at(wstr);
	return UINT16_MAX;
}