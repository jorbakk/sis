
#pragma once


// #include <liblocate/liblocate_api.h>
#define LIBLOCATE_API

#ifdef __cplusplus
extern "C"
{
#endif


/**
*  @brief
*    Get path to the current executable
*
*  @param[out] path
*    Path to executable (including filename)
*  @param[out] pathLength
*    Number of characters of path without null byte
*
*  @remark
*    The caller takes memory ownership over *path.
*/
LIBLOCATE_API void getExecutablePath(char ** path, unsigned int * pathLength);

/**
*  @brief
*    Get path to the current application bundle
*
*  @param[out] path
*    Path to bundle (including filename)
*  @param[out] pathLength
*    Number of characters of path without null byte
*
*  @remark
*    If the current executable is part of a macOS application bundle,
*    this function returns the part to the bundle. Otherwise, an
*    empty string is returned.
*
*  @remark
*    The caller takes memory ownership over *path.
*/
LIBLOCATE_API void getBundlePath(char ** path, unsigned int * pathLength);

/**
*  @brief
*    Get path to the current module
*
*  @param[out] path
*    Path to module (directory in which the executable is located)
*  @param[out] pathLength
*    Number of characters of path without null byte
*
*  @remark
*    The caller takes memory ownership over *path.
*/
LIBLOCATE_API void getModulePath(char ** path, unsigned int * pathLength);

/**
*  @brief
*    Get path to dynamic library
*
*  @param[in] symbol
*    A symbol from the library, e.g., a function or variable pointer
*  @param[out] path
*    Path to library (including filename)
*  @param[out] pathLength
*    Length of path
*
*  @remark
*    If symbol is null pointer, an empty string is returned.
*
*  @remark
*    The caller takes memory ownership over *path.
*/
LIBLOCATE_API void getLibraryPath(void * symbol, char ** path, unsigned int * pathLength);

/**
*  @brief
*    Locate path to a file or directory
*
*  @param[out] path
*    Path to file or directory
*  @param[out] pathLength
*    Length of path
*  @param[in] relPath
*    Relative path to a file or directory (e.g., 'data/logo.png')
*  @param[in] relPathLength
*    Length of relPath
*  @param[in] systemDir
*    Subdirectory for system installs (e.g., 'share/myappname')
*  @param[in] systemDirLength
*    Length of systemDir
*  @param[in] symbol
*    A symbol from the library, e.g., a function or variable pointer
*
*  @remark
*    This function tries to locate the named file or directory based
*    on the location of the current executable or library. If the
*    file or directory could be found, the base path from which the
*    relative path can be resolved is returned. Otherwise, an empty
*    string is returned.
*
*  @remark
*    The caller takes memory ownership over *path.
*/
LIBLOCATE_API void locatePath(char ** path, unsigned int * pathLength, const char * relPath, unsigned int relPathLength,
    const char * systemDir, unsigned int systemDirLength, void * symbol);

/**
*  @brief
*    Get platform specific path separator
*
*  @param[out] sep
*    Path separator (e.g., '`/`' or '`\`')
*/
LIBLOCATE_API void pathSeparator(char * sep);

/**
*  @brief
*    Get platform specific shared library prefix (e.g., 'lib' on UNIX systems, '' on Windows)
*
*  @param[out] prefix
*    Library prefix
*  @param[out] prefixLength
*    Length of prefix
*
*  @remark
*    The caller takes memory ownership over *prefix.
*/
LIBLOCATE_API void libPrefix(char ** prefix, unsigned int * prefixLength);

/**
*  @brief
*    Get main platform specific shared library extension (e.g., 'dll', 'dylib', or 'so')
*
*  @param[out] extension
*    Library extension
*  @param[out] extensionLength
*    Length of extension
*
*  @remark
*    The caller takes memory ownership over *extension.
*/
LIBLOCATE_API void libExtension(char ** extension, unsigned int * extensionLength);

/**
*  @brief
*    Get platform specific shared library extensions (e.g., ['dll'], ['so'], or ['so', 'dylib'])
*
*  @param[out] extensions
*    Library extensions
*  @param[out] extensionLengths
*    Length of extensions
*  @param[out] extensionCount
*    Number of extensions (the length of both arrays)
*
*  @remark
*    The caller takes memory ownership over *extensions and every string pointer within as well as *extensionLengths.
*/
LIBLOCATE_API void libExtensions(char *** extensions, unsigned int ** extensionLengths, unsigned int * extensionCount);

/**
*  @brief
*    Get home directory of the current user
*
*  @param[out] dir
*    Home directory
*  @param[out] dirLength
*    Length of directory
*
*  @remark
*    The caller takes memory ownership over *dir.
*/
LIBLOCATE_API void homeDir(char ** dir, unsigned int * dirLength);

/**
*  @brief
*    Get profile directory of the current user
*
*  @param[out] dir
*    Profile directory
*  @param[out] dirLength
*    Length of directory
*
*  @remark
*    The caller takes memory ownership over *dir.
*/
LIBLOCATE_API void profileDir(char ** dir, unsigned int * dirLength);

/**
*  @brief
*    Get document directory of the current user
*
*  @param[out] dir
*    Document directory
*  @param[out] dirLength
*    Length of directory
*
*  @remark
*    The caller takes memory ownership over *dir.
*/
LIBLOCATE_API void documentDir(char ** dir, unsigned int * dirLength);

/**
*  @brief
*    Get roaming directory for the named application
*
*  @param[out] dir
*    Roaming directory
*  @param[out] dirLength
*    Length of directory
*  @param[in] application
*    Application name
*  @param[in] applicationLength
*    Length of application name
*
*  @remark
*    The caller takes memory ownership over *dir.
*/
LIBLOCATE_API void roamingDir(char ** dir, unsigned int * dirLength, const char * application, unsigned int applicationLength);

/**
*  @brief
*    Get local directory for the named application
*
*  @param[out] dir
*    Local directory
*  @param[out] dirLength
*    Length of directory
*  @param[in] application
*    Application name
*  @param[in] applicationLength
*    Length of application name
*
*  @remark
*    The caller takes memory ownership over *dir.
*/
LIBLOCATE_API void localDir(char ** dir, unsigned int * dirLength, const char * application, unsigned int applicationLength);

/**
*  @brief
*    Get config directory for the named application
*
*  @param[out] dir
*    Config directory
*  @param[out] dirLength
*    Length of directory
*  @param[in] application
*    Application name
*  @param[in] applicationLength
*    Length of application name
*
*  @remark
*    The caller takes memory ownership over *dir.
*/
LIBLOCATE_API void configDir(char ** dir, unsigned int * dirLength, const char * application, unsigned int applicationLength);

/**
*  @brief
*    Get temporary directory for the named application
*
*  @param[out] dir
*    Temporary directory
*  @param[out] dirLength
*    Length of directory
*  @param[in] application
*    Application name
*  @param[in] applicationLength
*    Length of application name
*
*  @remark
*    The caller takes memory ownership over *dir.
*/
LIBLOCATE_API void tempDir(char ** dir, unsigned int * dirLength, const char * application, unsigned int applicationLength);


unsigned char checkStringParameter(const char * path, unsigned int * pathLength);
unsigned char checkStringOutParameter(char ** path, unsigned int * pathLength);
unsigned char checkStringVectorOutParameter(char *** paths, unsigned int ** lengths, unsigned int * count);
void invalidateStringOutParameter(char ** path, unsigned int * pathLength);
void copyToStringOutParameter(const char * source, unsigned int length, char ** target, unsigned int * targetLength);

/**
*  @brief
*    Convert path into unified form (replace '\' with '/')
*
*  @param[inout] path
*    Path
*  @param[in] pathLength
*    The length of path
*/
void unifyPathDelimiters(char * path, unsigned int pathLength);

/**
*  @brief
*    Cut away filename portion of a path, get path to directory
*
*  @param[in] fullpath
*    Path (e.g., '/path/to/file.txt')
*  @param[in] length
*    Length of fullpath (excluding null byte)
*  @param[out] newLength
*    The end of the substring representing the directory path (e.g., '/path/to')
*/
void getDirectoryPart(const char * fullpath, unsigned int length, unsigned int * newLength);

/**
*  @brief
*    Cut away filename and bundle portion of a path, get path to bundle root
*
*  @param[in] fullpath
*    Path (e.g., '/path/to/file.txt')
*  @param[in] length
*    Length of fullpath (excluding null byte)
*  @param[out] newLength
*    The end of the substring representing the directory path (e.g., '/path/to')
*/
void getBundlePart(const char * fullpath, unsigned int length, unsigned int * newLength);

/**
*  @brief
*    Get system base path for path to library or executable
*
*  @param[in] path
*    Path to library or executable (e.g., '/usr/bin/myapp')
*  @param[in] pathLength
*    The length of the path
*  @param[out] systemPathLength
*    The last position of the system part of the path (e.g., '/usr')
*
*  @remarks
*    This function returns the base path if the given file
*    is a system path. Otherwise, it returns an empty string.
*
*    Examples:
*      '/usr/bin/myapp' -> '/usr'
*      '/usr/local/bin/myapp' -> '/usr/local'
*      '/usr/lib/mylib.so' -> '/usr'
*      '/usr/lib64/mylib.so' -> '/usr'
*      '/usr/local/lib64/mylib.so' -> '/usr/local'
*      '/crosscompile/armv4/usr/lib/mylib.so.2' -> '/crosscompile/armv4/usr'
*/
void getSystemBasePath(const char * path, unsigned int pathLength, unsigned int * systemPathLength);

/**
*  @brief
*    Get value of environment variable
*
*  @param[in] name
*    Name of environment variable
*  @param[in] nameLength
*    The length of name
*  @param[out] value
*    The value of the referenced environment variable
*  @param[out] valueLength
*    The length of value
*
*  @return
*    Value of the environment variable
*
*  The caller takes memory ownership over *value.
*/
void getEnv(const char * name, unsigned int nameLength, char ** value, unsigned int * valueLength);

/**
*  @brief
*    Check if file or directory exists
*
*  @param[in] path
*    Path to file or directory
*  @param[in] pathLength
*    Length of path
*
*  @return
*    'true' if it exists, else 'false'
*/
unsigned char fileExists(const char * path, unsigned int pathLength);


#ifdef __cplusplus
} // extern "C"
#endif
