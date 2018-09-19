#include <ork/timer.h>
#include <ork/ipcq.h>
#include <ork/thread.h>
#include <unittest++/UnitTest++.h>
#include <string.h>
#include <math.h>
#include <set>

typedef ork::IpcPacket_t msg_t;

static const int begin_message = 0;
static const int continue_message = 1;
static const int end_message = 2;
static const int kill_message = 3;
static const int max_payload_size = ork::ipcq_msglen-32;
static const int max_xact_size = 262144;

ork::IpcMsgQSender ipcqs;
ork::IpcMsgQReciever ipcqr;
     
std::atomic<int> msgcounter;

void send_loop(const std::string& ipc_name)
{   printf( "IpcMsgQSender creating @ ipc_name<%s>...\n", ipc_name.c_str() );
    ipcqs.Create(ipc_name);
    printf( "IpcMsgQSender created (and connected)...\n");

    ////////////////////////////////////////////
    // initialize data 
    ////////////////////////////////////////////

    const size_t length = max_xact_size;
    uint8_t data[length];

    for( size_t i=0; i<length; i++ )
        data[i] = uint8_t(i&0xff);

    ork::Timer t;
    t.Start();
    ////////////////////////////////////////////
    // tight send loop
    ////////////////////////////////////////////

    std::set<int> sizes = { 1,2,4,8,16,32,64,128,256,511 };

    for( int i : sizes )
    {
        bool iter_done = false;

        size_t byte_counter = 0;
        int size_this_iter = i*64;
        printf("\nsend iter message_size<%d>\n", size_this_iter );

        t.Start();
        int checkiter = 10000;

        while(false==iter_done)
    	{   

            auto& msg1 = ipcqs.beginSendPacket();
            msg1.Write<int>(begin_message);
            msg1.Write<int>(size_this_iter);
            ipcqs.endSendPacket(); 

            auto src_ptr = data;
            int index = 0;
            
            while( index < size_this_iter )
            {   auto& msg2 = ipcqs.beginSendPacket();
                msg2.Write<int>(continue_message);
                int size_this_packet = (size_this_iter-index);
                if( size_this_packet>max_payload_size )
                    size_this_packet = max_payload_size;
                msg2.Write<int>(size_this_packet);
                msg2.WriteData(src_ptr,size_this_packet);
                ipcqs.endSendPacket(); 
                index += size_this_packet;
                src_ptr += size_this_packet;
            }
            auto& msg3 = ipcqs.beginSendPacket();
            msg3.Write<int>(end_message);
            ipcqs.endSendPacket(); 

            byte_counter += size_this_iter;

            if( checkiter-- == 0 ){
                iter_done = t.SecsSinceStart()>5.0f;
                checkiter = 10000;
            }
        }
    }

    msg_t msg;
    msg.Write<int>(kill_message);
    ipcqs.sendPacket(msg); 

    ////////////////////////////////////////////
    // cleanup
    ////////////////////////////////////////////

    printf( "\nIpcMsgQSender going down...\n");
}

void recv_loop(const std::string& ipc_name) {
    ////////////////////////////////////////////
    // recv init
    ////////////////////////////////////////////
    printf( "IpcMsgQReciever connecting @ ipc_name<%s>...\n", ipc_name.c_str() );
    ipcqr.Connect(ipc_name);
    printf( "IpcMsgQReciever connected...\n");
    bool ok_to_exit = false;
    const size_t buflen = max_xact_size;
    uint8_t buffer[buflen];
    int index = 0;
    ////////////////////////////////////////////
    // tight recv loop
    ////////////////////////////////////////////
    int message_size = 0;
    while(false==ok_to_exit)
    {   if( auto msg = ipcqr.beginRecvPacket() )
        {   msg_t::iter_t recv_it(*msg);
            int icmd = -1;
            msg->Read<int>(icmd,recv_it);
            switch( icmd )
            {   case begin_message:
                {   index = 0;
                    msg->Read<int>(message_size,recv_it);
                    //assert(message_size<buflen);
                    //printf("recv message_size<%d>\n", message_size );
                    break;
                }
                case continue_message:
                {   int thisiter = 0;
                    msg->Read<int>(thisiter,recv_it);
                    //printf("recv index<%d> thisiter<%d>\n", index, thisiter );
                    assert((index+thisiter)<=message_size);
                    msg->ReadData(buffer+index,thisiter,recv_it);
                    index += thisiter;
                    break;
                }
                case end_message:
                    break;
                case kill_message:
                    ok_to_exit = true;
                    break;
                default:
                    assert( false );
             }
             ipcqr.endRecvPacket();
        }
        else
            usleep(100);
    }
    ////////////////////////////////////////////
    printf( "\nIpcMsgQReciever going down...\n");
}

TEST(ipcq1)
{
	ork::thread sendthr("sendthr");
	ork::thread recvthr("recvthr");

    auto ipcname = ork::FormatString("/ipcq_yo");

    bool ok_to_exit = false;

    sendthr.start([ipcname](){
    	send_loop(ipcname);
    });

    recvthr.start([ipcname,&ok_to_exit](){
    	recv_loop(ipcname);
        ok_to_exit = true;
    });


    while(false==ok_to_exit)
    {
        auto prof_frame = ipcqs.profile();
        double mbps = double(prof_frame._bytesSent)/1048576.0;
        double msgps = prof_frame._messagesSent;
        printf( "Msg/Sec<%d> MiB/Sec<%d>       \r", int(msgps), int(mbps) );
        fflush(stdout);
    	usleep(1000000);
    }
}
