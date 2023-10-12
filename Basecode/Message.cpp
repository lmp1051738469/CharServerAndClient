#include"Message.h"
#include<cstdio>

void Message::save_msg(std::stringstream &sa)
{
    //发送时，把对象转为流对象，然后转为const char*
    boost::archive::text_oarchive oa(sa); 
    oa << *this; 
}

void Message::load_msg( std::stringstream &sa)
{
    boost::archive::text_iarchive ia(sa);  
    ia >> *this; 
}

void Message::send_content( std::stringstream &sa )
{
    save_msg( sa );
}

void Message::read_content(std::stringstream &sa)
{  
    load_msg(sa);
}
