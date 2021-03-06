//
// Created by michiel on 24-1-2016.
//

#include "output.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
namespace OpenPSTD
{
    namespace CLI
    {
        using namespace Kernel;

        void CLIOutput::Callback(CALLBACKSTATUS status, std::string message, int frame)
        {
            if (status == CALLBACKSTATUS::STARTING)
            {
                std::cout << message << std::endl;
            }
            else if (status == CALLBACKSTATUS::RUNNING)
            {
                std::cout << message << std::endl;
            }
            else if (status == CALLBACKSTATUS::FINISHED)
            {
                std::cout << message << std::endl;
            }
        }

        void CLIOutput::WriteFrame(int frame, int domain, PSTD_FRAME_PTR data)
        {
            _file->SaveNextResultsFrame(domain, data);
        }

        void CLIOutput::WriteSample(int startSample, int receiver, std::vector<float> data)
        {
            Kernel::PSTD_RECEIVER_DATA_PTR data_ptr = std::make_shared<Kernel::PSTD_RECEIVER_DATA>(data);
            _file->SaveReceiverData(receiver, data_ptr);
        }
    }
}