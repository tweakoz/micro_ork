#include <ork/timer.h>
#include <ork/ipcq.h>
#include <ork/thread.h>
#include <unittest++/UnitTest++.h>
#include <string.h>
#include <math.h>

typedef ork::IpcPacket_t msg_t;

static const int begin_message = 0;
static const int continue_message = 1;
static const int end_message = 2;
static const int max_payload_size = ork::ipcq_msglen-32;
static const int xact_size = 262144;

ork::IpcMsgQSender ipcqs;
ork::IpcMsgQReciever ipcqr;
     
std::atomic<int> msgcounter;

void send_loop(const std::string& ipc_name)
{   printf( "IpcMsgQSender creating @ ipc_name<%s>...\n", ipc_name.c_str() );
    ipcqs.Create(ipc_name);
    printf( "IpcMsgQSender created (and connected)...\n");
    bool ok_to_exit = false;

    ////////////////////////////////////////////
    // initialize data 
    ////////////////////////////////////////////

    const size_t length = xact_size;
    uint8_t data[length];

    for( size_t i=0; i<length; i++ )
        data[i] = uint8_t(i&0xff);

    ////////////////////////////////////////////
    // tight send loop
    ////////////////////////////////////////////

    while(false==ok_to_exit)
	{   msg_t msg;
        msg.Write<int>(begin_message);
        ipcqs.send(msg); 
        auto src_ptr = data;
        int index = 0;
        while( index < length )
        {   msg.clear();
            msg.Write<int>(continue_message);
            int thisiter = (length-index);
            if( thisiter>max_payload_size )
                thisiter = max_payload_size;
            msg.Write<int>(thisiter);
            msg.WriteData(src_ptr,thisiter);
            ipcqs.send(msg); 
            index += thisiter;
            src_ptr += thisiter;
        }
        msg.clear();
        msg.Write<int>(end_message);
        ipcqs.send(msg); 
    }

    ////////////////////////////////////////////
    // cleanup
    ////////////////////////////////////////////

    printf( "IpcMsgQSender going down...\n");
}

void recv_loop(const std::string& ipc_name) {
    ////////////////////////////////////////////
    // recv init
    ////////////////////////////////////////////
    printf( "IpcMsgQReciever connecting @ ipc_name<%s>...\n", ipc_name.c_str() );
    ipcqr.Connect(ipc_name);
    printf( "IpcMsgQReciever connected...\n");
    bool ok_to_exit = false;
    const size_t buflen = xact_size;
    uint8_t buffer[buflen];
    int index = 0;
    ////////////////////////////////////////////
    // tight recv loop
    ////////////////////////////////////////////
    while(false==ok_to_exit)
    {   msg_t msg;
        bool got = ipcqr.try_recv(msg);
        if( got )
        {   msg_t::iter_t recv_it(msg);
            int icmd = -1;
            msg.Read<int>(icmd,recv_it);
            switch( icmd )
            {   case begin_message:
                {   index = 0;
                    break;
                }
                case continue_message:
                {   int thisiter = 0;
                    msg.Read<int>(thisiter,recv_it);
                    assert((index+thisiter)<=buflen);
                    msg.ReadData(buffer+index,thisiter,recv_it);
                    index += thisiter;
                    break;
                }
                case end_message:
                    break;
                default:
                    assert( false );
             }
        }
        else
            usleep(100);
    }
    ////////////////////////////////////////////
    printf( "IpcMsgQReciever going down...\n");
}

TEST(ipcq1)
{
	ork::thread sendthr("sendthr");
	ork::thread recvthr("recvthr");

    auto ipcname = ork::FormatString("/ipcq_yo");

    sendthr.start([ipcname](){
    	send_loop(ipcname);
    });

    usleep(1<<20);

    recvthr.start([ipcname](){
    	recv_loop(ipcname);
    });

    bool ok_to_exit = false;

    while(false==ok_to_exit)
    {
        auto prof_frame = ipcqs.profile();
        double mbps = double(prof_frame._bytesSent)/1048576.0;
        double msgps = prof_frame._messagesSent;
        printf( "Msg/Sec<%4.2f> MiB/Sec<%4.2f>       \r", msgps, mbps );
        fflush(stdout);
    	usleep(1000000);
    }
}
