#include <ork/thread.h>
#include <sys/prctl.h>

namespace ork {

thread::thread(const std::string& name)
    : _name(name)
{
    mState = 0;
}
thread::~thread()
{
    join();
}

void* thread_impl(void* pdat)
{
    thread* pthr = (thread*) pdat;

    //printf( "starting thread<%p:%s>\n", pdat,pthr->_name.c_str() );
    SetCurrentThreadName(pthr->_name.c_str());

    pthr->mLambda();
    //printf( "thread<%p> returned\n", pdat );
    return nullptr;
}
#if defined(USE_SPROC)
void sproc_thread_impl(void* pdat)
{
    thread_impl(pdat);
}
#endif

void thread::start( const ork::void_lambda_t& l )
{
    int istate = mState.fetch_and_increment();
    //printf( "start thread<%p> istate<%d>\n", this, istate );
    if( istate==0 )
    {
        mLambda = l;
        #if defined(USE_PTHREAD)

        //pthread_attr_t attr;
        //int ia = pthread_attr_init(&attr);
        //pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS );
        //pthread_attr_setschedpolicy(&attr, SCHED_RR );
        //pthread_setconcurrency(2);

        pthread_create(&mThreadHandle, nullptr, thread_impl, (void*) this );
        //pthread_create(&mThreadHandle, &attr, thread_impl, (void*) this );
        #elif defined(USE_SPROC)
        mSprocPid = sproc( sproc_thread_impl, PR_SALL, (void*) this );
        printf( "mSprocPid<%d>\n", mSprocPid );
        sysmp(MP_RUNANYWHERE_PID, mSprocPid);
        /*while( int(mState) != 2 )
        {
            usleep(1000);
        }*/
        #endif
    }
}

void thread::join()
{
    if( mState.fetch_and_decrement()==1 )
    {
        #if defined(USE_PTHREAD)
        pthread_join(mThreadHandle,NULL);
        #elif defined(USE_SPROC)
        int status;
        waitpid((pid_t)mSprocPid, &status, 0);
        #endif
    }
    else
    {
        mState.fetch_and_increment();
    }
}

///////////////////////////////////////////////////////////////////////////

void SetCurrentThreadName(const char* threadName)
{
    static const int  kMAX_NAME_LEN = 15;
    char name[kMAX_NAME_LEN+1];
    for( int i=0; i<kMAX_NAME_LEN; i++ ) name[i]=0;
    strncpy(name,threadName,kMAX_NAME_LEN);
    name[kMAX_NAME_LEN]=0;
#if defined(OSX)
    pthread_setname_np(name); 
#elif defined(LINUX)
    prctl(PR_SET_NAME,(unsigned long)&name);
#endif
}

}
