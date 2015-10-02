//
// Created by omar on 17-9-15.
//

#include "Receiver.h"

namespace Kernel {
    Receiver::Receiver(std::vector<double> location,std::shared_ptr<PSTDFile> config, std::string id,
                       std::shared_ptr<Domain> container) : x(location.at(0)), y(location.at(1)), z(location.at(2)) {
        this->config = config;
        this->location = location;
        this->grid_location = std::make_shared<Point>(Point(this->x, this->y, this->z));
    }

    double Receiver::compute_local_pressure() {
        if (this->config->GetSettings().getSpectralInterpolation()) {
            return this->compute_with_si();
        } else {
            return this->compute_with_nn();
        }
    }

    double Receiver::compute_fft_factor(Point size, BoundaryType bt) {
        int primary_dimension = 0;
        if (bt == BoundaryType.HORIZONTAL) {
            primary_dimension = size.x;
        } else {
            primary_dimension = size.y;
        }
        //Todo: Is this the wave length number?
        wave_length_number = 2 * this->config->GetSettings()->getWaveLength() + primary_dimension + 1;
        //Pressure grid is staggered, hence + 1
        //Todo: Finish this

        return 0;

    }

    double Receiver::compute_with_nn() {

    }

    double Receiver::compute_with_si() {

    }
}