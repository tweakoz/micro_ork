#include <ork/path.h>
#include <unistd.h>
#include <glob.h>
#include <fts.h>
#include <errno.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

static int wildcmp( const char *wild, const char *string)
{   const char *cp = 0, *mp = 0;
    while ((*string) && (*wild != '*'))
    {   if ((*wild != *string) && (*wild != '?'))
        {   return 0;
        }
        wild++;
        string++;
    }
    while (*string)
    {   if (*wild == '*')
        {   if (!*++wild)
            {   return 1;
            }
            mp = wild;
            cp = string+1;
        }
        else if ((*wild == *string) || (*wild == '?'))
        {   wild++;
            string++;
        }
        else
        {   wild = mp;
            string = cp++;
        }
    }
    while (*wild == '*')
    {   wild++;
    }
    return !*wild;
}

///////////////////////////////////////////////////////////////////////////////

filename_list_t glob( const Path::NameType & wildcards, const ork::Path& initdir )
{
    filename_list_t rval;
    
    Path::NameType _wildcards = wildcards;
    if( _wildcards == (Path::NameType) "" )
        _wildcards = (Path::NameType) "*";

#if defined( IX ) 

    const char* path = initdir.c_str();
    char* const paths[]= 
    {
        (char* const) path,
        0
    };

    FTS *tree = fts_open(&paths[0], FTS_NOCHDIR, 0);
    if (!tree) {
        perror("fts_open");
        OrkAssert(false);
        return rval;
    }

    FTSENT *node;
    while ((node = fts_read(tree)))
    {
        if (node->fts_level > 0 && node->fts_name[0] == '.')
            fts_set(tree, node, FTS_SKIP);
        else if (node->fts_info & FTS_F) {
            
            
            int match = wildcmp( _wildcards.c_str(), node->fts_name );
            if( match )
            {
                Path::NameType fullname;
                fullname.replace(node->fts_accpath,"//","/");
                rval.push_back( fullname );
                //orkprintf( "file found <%s>\n", fullname.c_str() );
            }

            
//          printf("got file named %s at depth %d, "
//          "accessible via %s from the current directory "
//          "or via %s from the original starting directory\n",
//          node->fts_name, node->fts_level,
//          node->fts_accpath, node->fts_path);
            /* if fts_open is not given FTS_NOCHDIR,
            * fts may change the program's current working directory */
        }
    }
    if (errno) {
        perror("fts_read");
        OrkAssert(false);
        return rval;
    }

    if (fts_close(tree)) {
        perror("fts_close");
        OrkAssert(false);
        return rval;
    }

#elif defined( WIN32 )

    Path::NameType srchdir( initdir.ToAbsolute(ork::Path::EPATHTYPE_DOS).c_str() );

    if( srchdir == (Path::NameType) "" )
        srchdir = (Path::NameType) "./";


    Path::NameType olddir = GetCurDir();
    DWORD ret;
    //HANDLE h = 0;
    std::transform( srchdir.begin(), srchdir.end(), srchdir.begin(), unix2winpathsep );

    std::stack<Path::NameType> dirstack;
    dirstack.push( srchdir );

    WIN32_FIND_DATA find_dirs;
    WIN32_FIND_DATA find_files;
    Path::NameType cd = (Path::NameType) ".";
    Path::NameType pd = (Path::NameType) "..";

    orkvector<Path::NameType> dirvect;

    while( dirstack.empty() == false )
    {   Path::NameType dir = dirstack.top();
        dirvect.push_back( dir );
        Path::NameType dirpath;
        for( orkvector<Path::NameType>::size_type i=0; i<dirvect.size(); i++ ) dirpath += dirvect[i];
        dirstack.pop();

        //orkmessageh( "processing dir %s\n", dir.c_str() );

        //////////////////////////////////////////////////////////////////////////////////
        // Process Files
        //////////////////////////////////////////////////////////////////////////////////

        Path::NameType searchspec_files = dir+wildcards;
        HANDLE h_files = FindFirstFile(searchspec_files.c_str(), &find_files);
        if( h_files==INVALID_HANDLE_VALUE )
        {   ret = GetLastError();
            if (ret != ERROR_NO_MORE_FILES)
            {   // TODO: return some error code
            }
        }
        else
        {   bool bcont = true;
            while(bcont)
            {   ork::msleep(0);
                Path::NameType name = find_files.cFileName;
                Path::NameType _8dot3name = find_files.cAlternateFileName;
                if (0 == (find_files.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                {
                    int match = wildcmp( _wildcards.c_str(), name.c_str() );
                    if( match )
                    {
                        Path::NameType fullname = dirpath+name;
                        rval.push_back( fullname );
                        //orkprintf( "file found <%s>\n", fullname.c_str() );
                    }
                }
                int err = FindNextFile(h_files, &find_files);
                bcont = (err!=0);
            }
        }
        if( h_files && h_files != HANDLE(0xffffffff) )
        {
            FindClose(h_files);
        }
        //////////////////////////////////////////////////////////////////////////////////
        // Process Dirs
        //////////////////////////////////////////////////////////////////////////////////

        Path::NameType searchspec_dirs = dir+"*";
        HANDLE h_dirs = FindFirstFile(searchspec_dirs.c_str(), &find_dirs);

        if( h_dirs==INVALID_HANDLE_VALUE )
        {   ret = GetLastError();
            if (ret != ERROR_NO_MORE_FILES)
            {   // TODO: return some error code
            }
        }
        else
        {   bool bcont = true;
            
            while(bcont)
            {   
                ork::msleep(0);
                Path::NameType name = find_dirs.cFileName;
                Path::NameType _8dot3name = find_dirs.cAlternateFileName;
                    
                if (find_dirs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {   // Append a trailing slash to directory names...
                    //strcat(selectDir->d_name, "/");
                    if( (name!=cd)&&(name!=pd) )
                    {   Path::NameType dirname = dirpath+name + (Path::NameType)"\\";
                        dirstack.push( dirname );
                        //orkmessageh( "pushing dir %s\n", dirname.c_str() );
                    }
                }
                int err = FindNextFile(h_dirs, &find_dirs);
                bcont = (err!=0);
            }
        }

        if( h_dirs && h_dirs != HANDLE(0xffffffff) )
        {
            FindClose(h_dirs);
        }
        
        //////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////

        dirvect.pop_back();
    }

    ////////////////////////////////////////////////////////////////////
    size_t ifiles = rval.size();
    ret = GetLastError();
    if (ret != ERROR_NO_MORE_FILES) {}  // TODO: return some error code
    ////////////////////////////////////////////////////////////////////
#endif
    return rval;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork