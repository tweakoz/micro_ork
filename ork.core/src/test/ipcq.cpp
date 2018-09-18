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

    ////////////////////////////////////////////
    // tight send loop
    ////////////////////////////////////////////

    for( int i=1; i<16; i++ )
    {
        bool iter_done = false;

        size_t byte_counter = 0;
        int size_this_iter = i*1024;
        printf("\nsend iter message_size<%d>\n", size_this_iter );

        while(false==iter_done)
    	{   

            msg_t msg;
            msg.Write<int>(begin_message);
            msg.Write<int>(size_this_iter);
            ipcqs.send(msg); 
            auto src_ptr = data;
            int index = 0;
            
            while( index < size_this_iter )
            {   msg.clear();
                msg.Write<int>(continue_message);
                int size_this_packet = (size_this_iter-index);
                if( size_this_packet>max_payload_size )
                    size_this_packet = max_payload_size;
                msg.Write<int>(size_this_packet);
                msg.WriteData(src_ptr,size_this_packet);
                ipcqs.send(msg); 
                index += size_this_packet;
                src_ptr += size_this_packet;
            }
            msg.clear();
            msg.Write<int>(end_message);
            ipcqs.send(msg); 

            byte_counter += size_this_iter;
            iter_done = (byte_counter>(16L<<30));
        }
    }

    msg_t msg;
    msg.Write<int>(kill_message);
    ipcqs.send(msg); 

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
    const size_t buflen = max_xact_size;
    uint8_t buffer[buflen];
    int index = 0;
    ////////////////////////////////////////////
    // tight recv loop
    ////////////////////////////////////////////
    int message_size = 0;
    while(false==ok_to_exit)
    {   msg_t msg;
        ipcqr.recv(msg);
        //if( got )
        {   msg_t::iter_t recv_it(msg);
            int icmd = -1;
            msg.Read<int>(icmd,recv_it);
            switch( icmd )
            {   case begin_message:
                {   index = 0;
                    msg.Read<int>(message_size,recv_it);
                    assert(message_size<buflen);
                    //printf("recv message_size<%d>\n", message_size );
                    break;
                }
                case continue_message:
                {   int thisiter = 0;
                    msg.Read<int>(thisiter,recv_it);
                    //printf("recv index<%d> thisiter<%d>\n", index, thisiter );
                    assert((index+thisiter)<=message_size);
                    msg.ReadData(buffer+index,thisiter,recv_it);
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
        }
        //else
        //    usleep(100);
    }
    ////////////////////////////////////////////
    printf( "IpcMsgQReciever going down...\n");
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
        printf( "Msg/Sec<%4.2f> MiB/Sec<%4.2f>       \r", msgps, mbps );
        fflush(stdout);
    	usleep(1000000);
    }
}
