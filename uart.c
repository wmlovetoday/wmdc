m_logger.LogInfo() <<"set="<<sample->seq<<" sec="<<sample->timeStamp.sec<<" len="<<sample->validLen;
m_logger.LogInfo() <<"sync="<<sample->data[0];

m_logger.LogInfo() <<"set="<<sample->seq<<" sec="<<sample->timeStamp.sec<<" to len="<<sample->validLen;
//m_logger.LogInfo() <<"sync="<<sample->data[0];
m_logger.LogInfo() <<"header len="<<sample->data[4]<<"msg id="<<sample->data[5]+sample->data[6]*8;
m_logger.LogInfo() <<"msg type="<<sample->data[7]<<"msg len="<<sample->data[8];
m_logger.LogInfo() <<"week="<<sample->data[14]+sample->data[15]*8;
m_logger.LogInfo() <<"con="<<static_cast<char*>(sample->data+sample->data[4]);
