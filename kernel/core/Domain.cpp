//////////////////////////////////////////////////////////////////////////
// This file is part of openPSTD.                                       //
//                                                                      //
// openPSTD is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU General Public License as published by //
// the Free Software Foundation, either version 3 of the License, or    //
// (at your option) any later version.                                  //
//                                                                      //
// openPSTD is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of       //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        //
// GNU General Public License for more details.                         //
//                                                                      //
// You should have received a copy of the GNU General Public License    //
// along with openPSTD.  If not, see <http://www.gnu.org/licenses/>.    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Date: 17-9-2015
//
// Authors:
//      Louis van Harten, Omar Richardson
//
//////////////////////////////////////////////////////////////////////////

#include "Domain.h"

using namespace std;
using namespace Eigen;
namespace Kernel {

    Domain::Domain(shared_ptr<PSTDFileSettings> settings, string id, const float alpha,
                   Point top_left, Point size, const bool is_pml,
                   shared_ptr<WaveNumberDiscretizer> wnd, map<Direction, EdgeParameters> edge_param_map,
                   const shared_ptr<Domain> pml_for_domain = shared_ptr<Domain>(nullptr)) {

        this->initialize_domain(settings, id, alpha, top_left, size, is_pml, wnd, edge_param_map, pml_for_domain);
    }

    Domain::Domain(shared_ptr<PSTDFileSettings> settings, string id, const float alpha,
                   vector<float> top_left_vector, vector<float> size_vector, const bool is_pml,
                   shared_ptr<WaveNumberDiscretizer> wnd, map<Direction, EdgeParameters> edge_param_map,
                   const shared_ptr<Domain> pml_for_domain = shared_ptr<Domain>(nullptr)) {
        Point top_left((int) top_left_vector.at(0), (int) top_left_vector.at(1));
        Point size((int) size_vector.at(0), (int) size_vector.at(1));
        this->initialize_domain(settings, id, alpha, top_left, size, is_pml, wnd, edge_param_map, pml_for_domain);
    }

    void Domain::initialize_domain(shared_ptr<PSTDFileSettings> settings, string id, const float alpha,
                                   Point top_left, Point size, const bool is_pml,
                                   shared_ptr<WaveNumberDiscretizer> wnd,
                                   map<Direction, EdgeParameters> edge_param_map,
                                   const shared_ptr<Domain> pml_for_domain) {
        this->settings = settings;
        this->top_left = top_left;
        this->size = size; // Remember PML domains have a fixed size.
        this->bottom_right = top_left + size;
        this->wnd = wnd;
        this->id = id;
        this->edge_param_map = edge_param_map;
        this->alpha = alpha; // Todo: Usually 1, and barely used. Push to settings when PML domain becomes subclass
        //Todo: (TK): Probably wrong, especially with two neighbouring PML domains
        this->impedance = -((sqrt(1 - alpha) + 1) / (sqrt(1 - alpha) - 1));
        if (is_pml) { // Ugly... Fix when possible
            this->pml_for_domain_list.push_back(pml_for_domain);
        }
        if (this->is_rigid()) {
            this->rho = 1E200;
        } else {
            this->rho = this->settings->GetDensityOfAir();
        }
        this->is_pml = is_pml;
        this->is_secondary_pml = false;
        for (auto domain:this->pml_for_domain_list) {
            if (domain->is_pml) {
                is_secondary_pml = true;
                // TK: Assert there is only one primary pml domain.
            }
        }
        this->current_values = {};
        this->previous_values = {};
        this->l_values = {};
        this->pml_arrays = {};
        this->clear_fields();
        this->clear_matrices();
        this->clear_pml_arrays();
        this->local = false;
        cout << "Initialized " << *this << endl;
    }

    // version of calc that would have a return value.
    ArrayXXf Domain::calc(CalcDirection bt, CalculationType ct, ArrayXcf dest) {
        vector<shared_ptr<Domain>> domains1, domains2;
        if (bt == CalcDirection::X) {
            domains1 = left;
            domains2 = right;
        } else {
            domains1 = bottom;
            domains2 = top;
        }

        vector<int> own_range = get_range(bt);

        ArrayXXf source;

        if (/*dest != nullptr ||*/ false) { //TODO FIXME Fix dest checking
            if( ct == CalculationType::VELOCITY ) {
                if (bt == CalcDirection::X) {
                    source = extended_zeros(0, 1);
                } else {
                    source = extended_zeros(1, 0);
                }
            } else {
                source = extended_zeros(0, 0);
            }
        } else {
            if ( ct == CalculationType::PRESSURE ) {
                source = this->current_values.p0;
            } else {
                if ( bt == CalcDirection::X) {
                    source = this->current_values.u0;
                } else {
                    source = this->current_values.w0;
                }
            }
        }

        // loop over all possible combinations of neighbours for this domain (including null on one side)
        shared_ptr<Domain> d1, d2;
        for (int i = 0; i != domains1.size() + 1; i++) { //the +1 because a null pointer is also needed
            d1 = (i != domains1.size()) ? domains1[i] : nullptr;
            for (int j = 0; i != domains2.size() + 1; j++) {
                d2 = (i != domains2.size()) ? domains2[i] : nullptr;
                vector<string> rho_matrix_key;
                rho_matrix_key.push_back(d1->id);
                rho_matrix_key.push_back(d2->id);

                //The range is determined and clipped to the neighbour domain ranges
                vector<int> range_intersection = own_range;

                if (d1 != nullptr) {
                    vector<int> range1 = d1->get_range(bt);
                    set_intersection(range1.begin(), range1.end(),
                                     range_intersection.begin(), range_intersection.end(),
                                     back_inserter(range_intersection));
                }
                if (d2 != nullptr) {
                    vector<int> range2 = d1->get_range(bt);
                    set_intersection(range2.begin(), range2.end(),
                                     range_intersection.begin(), range_intersection.end(),
                                     back_inserter(range_intersection));
                }

                // If there is nothing left after clipping to domains, continue with a different set of domains
                if (range_intersection.size() == 0) {
                    continue;
                }

                // Set up various parameters and intermediates that are needed for the spatial derivatives
                int range_start = *min_element(range_intersection.begin(), range_intersection.end());
                int range_end = *max_element(range_intersection.begin(), range_intersection.end()) + 1;
                int primary_dimension = (bt == CalcDirection::X) ? size.x : size.y;
                int N_total = 2 * settings->GetWindowSize() + primary_dimension;

                if (ct == CalculationType::PRESSURE) {
                    N_total++;
                } else {
                    primary_dimension++;
                }

                ArrayXXf matrix0, matrix1, matrix2;
                if (ct == CalculationType::VELOCITY && d1 == nullptr && d2 == nullptr) {
                    // For a PML layer parallel to its interface direction the matrix is concatenated with zeros
                    // TODO Louis: Why only zeroes for ct==velocity? Should zeroes also be added in the else{} block?
                    // a PML domain can also have a neighbour, see:
                    //   |             |
                    // __|_____________|___
                    //   |     PML     |
                    //  <--------------->
                    d1 = d2 = shared_from_this();
                    if (bt == CalcDirection::X) {
                        matrix1 = extended_zeros(0, 1);
                        matrix2 = extended_zeros(0, 1);
                    } else {
                        matrix1 = extended_zeros(1, 0);
                        matrix2 = extended_zeros(1, 0);
                    }
                } else {
                    if (d1 == nullptr) {
                        d1 = shared_from_this();
                    }
                    if (d2 == nullptr) {
                        d2 = shared_from_this();
                    }
                }

                if (ct == CalculationType::PRESSURE) {
                    matrix0 = current_values.p0;
                } else if (bt == CalcDirection::X) {
                    matrix0 = current_values.u0;
                } else {
                    matrix0 = current_values.w0;
                }

                // If the matrices are _not_ already filled with zeroes, choose which values to fill them with.
                if (matrix1.isZero()) { // Todo: Trouble
                    if (ct == CalculationType::PRESSURE) {
                        matrix1 = d1->current_values.p0;
                    } else {
                        if (bt == CalcDirection::X) {
                            matrix1 = d1->current_values.u0;
                        } else {
                            matrix1 = d1->current_values.w0;
                        }
                    }
                }
                if (matrix2.isZero()) { // Todo: Trouble
                    if (ct == CalculationType::PRESSURE) {
                        matrix2 = d2->current_values.p0;
                    } else {
                        if (bt == CalcDirection::X) {
                            matrix2 = d2->current_values.u0;
                        } else {
                            matrix2 = d2->current_values.w0;
                        }
                    }
                }

                // Set up parameters for the spatial derivative later
                int var_index = 0, direction = 0;
                if (ct == CalculationType::VELOCITY) {
                    var_index = 1;
                }
                if (bt == CalcDirection::X) {
                    direction = 1;
                }

                ArrayXcf derfact;
                if (dest.isZero()) { // Todo: Fix
                    derfact = dest;
                } else {
                    if (ct == CalculationType::PRESSURE) {
                        derfact = wnd->get_discretization(settings->GetGridSpacing(),
                                                          N_total).pressure_deriv_factors;
                    } else {
                        derfact = wnd->get_discretization(settings->GetGridSpacing(),
                                                          N_total).velocity_deriv_factors;
                    }
                }

                // Determine which rho matrix instance to use
                string rho_array_id;
                if (d1 != nullptr) {
                    if (d2 != nullptr) {
                        rho_array_id = d1->id + id + d2->id;
                    } else {
                        rho_array_id = d1->id + id;
                    }
                } else {
                    if (d2 != nullptr) {
                        rho_array_id = id + d2->id;
                    } else {
                        rho_array_id = id;
                    }
                }
                Array<float, 4, 2> rho_array;
                if (ct == CalculationType::PRESSURE) {
                    rho_array = rho_arrays[rho_array_id].pressure;
                } else {
                    rho_array = rho_arrays[rho_array_id].velocity;
                }

                //source
                //TODO: set matrix to result of spatderp3
            }
        }
    }

    /**
     * Near-alias to calc(BoundaryType bt, CalculationType ct, vector<float> dest), but with
     * a default empty vector as dest.
     */
    void Domain::calc(CalcDirection bt, CalculationType ct) {
        //Domain::calc(bt, ct, nullptr); //Todo: Fix this call
    }

    bool Domain::contains_point(Point point) {
        vector<float> location = {(float) point.x, (float) point.y, (float) point.z};
        return contains_location(location);
    }

    bool Domain::contains_location(vector<float> location) {
        for (unsigned long dim = 0; dim < location.size(); dim++) {
            if (top_left.array.at(dim) > location.at(dim) or
                location.at(dim) > bottom_right.array.at(dim)) {
                return false;
            }
        }
        return true;
    }

    bool Domain::is_neighbour_of(shared_ptr<Domain> domain) {
        for (Direction direction :all_directions) {
            auto dir_nb = get_neighbours_at(direction);
            if (find(dir_nb.begin(), dir_nb.end(), domain) != dir_nb.end()) {
                return true;
            }
        }
        return false;
    }

    bool Domain::is_pml_for(shared_ptr<Domain> domain) {
        return (find(pml_for_domain_list.begin(), pml_for_domain_list.end(), domain) != pml_for_domain_list.end());
    }

    bool Domain::is_rigid() {
        return impedance > 1000; //Why this exact value?
    }

    vector<int> Domain::get_range(CalcDirection bt) {
        int a_l, b_l;
        if (bt == CalcDirection::X) {
            a_l = top_left.x;
            b_l = bottom_right.x;
        } else {
            a_l = top_left.y;
            b_l = bottom_right.y;
        }
        vector<int> tmp((unsigned long) (b_l - a_l));
        iota(tmp.begin(), tmp.end(), a_l); // Increases with 1 from a_l
        return tmp;
    }

    vector<int> Domain::get_intersection_with(shared_ptr<Domain> other_domain, Direction direction) {
        vector<int> own_range;
        vector<int> other_range;
        switch (direction) {
            case Direction::LEFT:
            case Direction::RIGHT:
                own_range = get_range(CalcDirection::X);
                other_range = other_domain->get_range(CalcDirection::X);
                break;
            case Direction::TOP:
            case Direction::BOTTOM:
                own_range = get_range(CalcDirection::Y);
                other_range = other_domain->get_range(CalcDirection::Y);
                break;
        }
        vector<int> range_intersection = own_range;
        set_intersection(other_range.begin(), other_range.end(), range_intersection.begin(), range_intersection.end(),
                         back_inserter(range_intersection));
        return range_intersection;
    }

    ArrayXXf Domain::extended_zeros(int x, int y, int z) {
        return ArrayXXf::Zero(size.x + x, size.y + y);
    }

    vector<shared_ptr<Domain>> Domain::get_neighbours_at(Direction direction) {
        switch (direction) {
            case Direction::LEFT:
                return left;
            case Direction::RIGHT:
                return right;
            case Direction::TOP:
                return top;
            case Direction::BOTTOM:
                return bottom;
        }
    }

    shared_ptr<Domain> Domain::get_neighbour_at(Direction direction, vector<float> location) {
        shared_ptr<Domain> correct_domain = nullptr;
        auto dir_neighbours = get_neighbours_at(direction);
        for (shared_ptr<Domain> domain:dir_neighbours) {
            if (domain->contains_location(location)) {
                assert(correct_domain == nullptr);
                correct_domain = domain;
            }
        }
        return correct_domain;
    }


    void Domain::compute_number_of_neighbours() {
        num_neighbour_domains = 0;
        num_pml_neighbour_domains = 0;
        for (Direction direction: all_directions) {
            for (auto domain: get_neighbours_at(direction)) {
                num_neighbour_domains++;
                if (domain->is_pml) {
                    num_pml_neighbour_domains++;
                }
            }
        }
    }

    int Domain::number_of_neighbours(bool count_pml) {
        if (count_pml) {
            return num_neighbour_domains;
        } else {
            return num_neighbour_domains - num_pml_neighbour_domains;
        }
    }

    void Domain::add_neighbour_at(shared_ptr<Domain> domain, Direction direction) {
        switch (direction) {
            case Direction::LEFT:
                left.push_back(domain);
                break;
            case Direction::RIGHT:
                right.push_back(domain);
                break;
            case Direction::TOP:
                top.push_back(domain);
                break;
            case Direction::BOTTOM:
                bottom.push_back(domain);
                break;
        }
    }

    void Domain::compute_rho_arrays() {
        //struct containing functions calculating the index strings to keep the for loop below somewhat readable.
        struct index_strings {
            string id(shared_ptr<Domain> d1, Domain *dm, shared_ptr<Domain> d2) {
                return d1->id + dm->id + d2->id;
            }

            string id(shared_ptr<Domain> d1, Domain *dm) {
                return d1->id + dm->id;
            }

            string id(Domain *dm, shared_ptr<Domain> d2) {
                return dm->id + d2->id;
            }

            string id(Domain *dm) {
                return dm->id;
            }
        };
        index_strings x;
        vector<shared_ptr<Domain>> left_domains = left;
        vector<shared_ptr<Domain>> right_domains = right;
        vector<shared_ptr<Domain>> top_domains = top;
        vector<shared_ptr<Domain>> bottom_domains = bottom;

        float max_rho = 1E10; // Large value, well within float range. Change for double.

        // Checks if sets of adjacent domains are non-zero and calculates the rho_arrays accordingly
        // TODO (optional) refactor: there is probably a prettier solution than if/else'ing this much
        if (left_domains.size()) {
            for (shared_ptr<Domain> d1 : left_domains) {
                if (right_domains.size()) {
                    for (shared_ptr<Domain> d2 : right_domains) {
                        vector<float> rhos = {d1->rho, d2->rho};
                        rho_arrays[x.id(d1, this, d2)] = get_rho_array(rhos[0], rho, rhos[1]);
                    }
                } else {
                    vector<float> rhos = {d1->rho, max_rho};
                    rho_arrays[x.id(d1, this)] = get_rho_array(rhos[0], rho, rhos[1]);
                }
            }
        } else {
            if (right_domains.size()) {
                for (shared_ptr<Domain> d2 : right_domains) {
                    vector<float> rhos = {max_rho, d2->rho};
                    rho_arrays[x.id(this, d2)] = get_rho_array(rhos[0], rho, rhos[1]);
                }
            } else {
                vector<float> rhos = {max_rho, max_rho};
                rho_arrays[x.id(this)] = get_rho_array(rhos[0], rho, rhos[1]);
            }
        }

        if (bottom_domains.size()) {
            for (shared_ptr<Domain> d1 : bottom_domains) {
                if (top_domains.size()) {
                    for (shared_ptr<Domain> d2 : top_domains) {
                        vector<float> rhos = {d1->rho, d2->rho};
                        rho_arrays[x.id(d1, this, d2)] = get_rho_array(rhos[0], rho, rhos[1]);
                    }
                } else {
                    vector<float> rhos = {d1->rho, max_rho};
                    rho_arrays[x.id(d1, this)] = get_rho_array(rhos[0], rho, rhos[1]);
                }
            }
        } else {
            if (top_domains.size()) {
                for (shared_ptr<Domain> d2 : top_domains) {
                    vector<float> rhos = {max_rho, d2->rho};
                    rho_arrays[x.id(this, d2)] = get_rho_array(rhos[0], rho, rhos[1]);
                }
            } else {
                vector<float> rhos = {max_rho, max_rho};
                rho_arrays[x.id(this)] = get_rho_array(rhos[0], rho, rhos[1]);
            }
        }
    }

    ArrayXXi Domain::get_vacant_range(Direction direction) {
        vector<shared_ptr<Domain>> neighbour_list;
        CalcDirection calc_dir = direction_to_calc_direction(direction);
        vector<int> range = get_range(calc_dir);
        set<int> range_set(range.begin(), range.end());
        switch (direction) {
            case Direction::LEFT:
                neighbour_list = left;
                break;
            case Direction::RIGHT:
                neighbour_list = right;
                break;
            case Direction::TOP:
                neighbour_list = top;
                break;
            case Direction::BOTTOM:
                neighbour_list = bottom;
                break;
        }
        for (shared_ptr<Domain> domain: neighbour_list) {
            vector<int> neighbour_range = domain->get_range(calc_dir);
            set<int> neighbour_range_set(neighbour_range.begin(), neighbour_range.end());
            set<int> set_diff;
            set_difference(range_set.begin(), range_set.end(), neighbour_range_set.begin(),
                           neighbour_range_set.end(), inserter(set_diff, set_diff.end()));
            range_set.swap(set_diff);
        }
        ArrayXXi vacant_range(range_set.size(), 2);
        range.assign(range_set.begin(), range_set.end());
        int iterator = 0;
        for (unsigned long i = 0; i < range.size(); i++) {
            if (i == 0 || range.at(i - 1) + 1 < range.at(i)) {
                vacant_range(0, 0) = range.at(i);
                vacant_range(0, 1) = range.at(i);
                iterator++;
            }
            if (i == range.size() - 1 || range.at(i + 1) - 1 > range.at(i)) {
                vacant_range(iterator - 1, 1) = range.at(i) + 1;
            }
        }
        return vacant_range.topRows(iterator);
    }


    void Domain::find_update_directions() {
        vector<CalcDirection> calc_directions = {CalcDirection::X, CalcDirection::Y};
        vector<Direction> directions = {Direction::LEFT, Direction::RIGHT, Direction::TOP, Direction::BOTTOM};
        for (CalcDirection calc_dir: calc_directions) {
            bool should_update = false;
            if (number_of_neighbours(false) == 1 and is_pml) {
                if (has_horizontal_attenuation and calc_dir == CalcDirection::X) {
                    // Todo: make sure we calculate in direction orthogonal to boundary
                    should_update = true;
                } else if (!has_horizontal_attenuation and calc_dir == CalcDirection::Y) {
                    should_update = true;
                } else if (local) {
                    should_update = false;
                } else {
                    for (Direction direction: directions) {
                        vector<shared_ptr<Domain>> dir_neighbours = get_neighbours_at(direction);
                        if (dir_neighbours.size() == 1 and !dir_neighbours.at(0)->is_pml) {
                            vector<shared_ptr<Domain>> opp_neighbours =
                                    dir_neighbours.at(0)->get_neighbours_at(get_opposite(direction));
                            assert(opp_neighbours.size() == 1 and id == opp_neighbours.at(0)->id);
                            should_update = !dir_neighbours.at(0)->edge_param_map[direction].locally_reacting;
                        }
                    }
                }
            }
            this->should_update[calc_dir] = should_update;
        }
    }

    void Domain::clear_matrices() {
        l_values.Lpx = extended_zeros(0, 1);
        l_values.Lpy = extended_zeros(1, 0);
        l_values.Lvx = extended_zeros(0, 0);
        l_values.Lvy = extended_zeros(0, 0);
    }

    void Domain::clear_fields() {
        current_values.p0 = extended_zeros(0, 0);
        current_values.px0 = extended_zeros(0, 0);
        current_values.py0 = extended_zeros(0, 0);
        current_values.u0 = extended_zeros(0, 1);
        current_values.w0 = extended_zeros(1, 0);

        previous_values = {}; // Do we def need to empty this?
    }


    void Domain::clear_pml_arrays() {
        pml_arrays.px = extended_zeros(0, 0);
        pml_arrays.py = extended_zeros(0, 0);
        pml_arrays.u = extended_zeros(0, 1);
        pml_arrays.w = extended_zeros(1, 0);

    }

    void Domain::compute_pml_matrices() {
        //Todo (0mar): Refactor this method? It's asymmetric and spaghetty
        /*
         * TK: Only calculate PML matrices for PML domains with a single non-pml neighbour
         * or for secondary PML domains with a single PML neighbour.
         */
        int number_normal_neighbour_domains = num_neighbour_domains - num_pml_neighbour_domains;
        if (is_secondary_pml) {
            is_corner_domain = num_neighbour_domains == 2;
            /*
             * TK: If this domain has its neighbour to the left or to the right,
             * a PML matrix is obtained for horizontal attenuation.
             */
            has_horizontal_attenuation = !left.empty() or !right.empty();
            if (is_corner_domain) {
                /**
                 * TK: Corner PML domains should have a horizontal as well as a vertical component.
                 * In particular: not to neighbours in the same direction.
                 */
                assert(has_horizontal_attenuation and (!top.empty() or !bottom.empty()));
                needs_reversed_attenuation = vector<bool>(); // Init necessary?
                needs_reversed_attenuation.push_back(!left.empty());
                needs_reversed_attenuation.push_back(!bottom.empty());
            } else {
                //TK: If this neighbour is located to the left or bottom, the attenuation is reversed.
                needs_reversed_attenuation.push_back(!left.empty() or !bottom.empty());
            }
        } else {
            is_corner_domain = false;
            /*
             * TK: If this domain has a neighbour to the left or right
             * and this neighbour is not a PML domain,
             * a PML matrix is obtained for horizontal (OR: Vertical?) attenuation.
             */
            bool all_air_left = !left.empty() and get_num_pmls_in_direction(Direction::LEFT) == 0;
            bool all_air_right = !right.empty() and get_num_pmls_in_direction(Direction::RIGHT) == 0;
            bool all_air_bottom = !bottom.empty() and get_num_pmls_in_direction(Direction::BOTTOM) == 0;

            has_horizontal_attenuation = all_air_left or all_air_right;
            needs_reversed_attenuation.push_back(all_air_left or all_air_bottom);
            if (is_secondary_pml and is_corner_domain) {
                // TK: PML is the product of horizontal and vertical attenuation.
                create_attenuation_array(CalcDirection::X, needs_reversed_attenuation.at(0),
                                         pml_arrays.px, pml_arrays.u);
                create_attenuation_array(CalcDirection::Y, needs_reversed_attenuation.at(1),
                                         pml_arrays.py, pml_arrays.u);
            } else {
                CalcDirection calc_dir = CalcDirection::Y;
                if (has_horizontal_attenuation) {
                    calc_dir = CalcDirection::X;
                }
                switch (calc_dir) {
                    case CalcDirection::X:
                        create_attenuation_array(calc_dir, needs_reversed_attenuation.at(0),
                                                 pml_arrays.px, pml_arrays.u);
                        pml_arrays.py = ArrayXXf::Ones(size.x, size.y);//Change if unique
                        pml_arrays.w = ArrayXXf::Ones(size.x, size.y);
                        break;
                    case CalcDirection::Y:
                        create_attenuation_array(calc_dir, needs_reversed_attenuation.at(0),
                                                 pml_arrays.py, pml_arrays.w);
                        pml_arrays.px = ArrayXXf::Ones(size.x, size.y);//Change if unique
                        pml_arrays.u = ArrayXXf::Ones(size.x, size.y);//Change if unique
                        break;
                }
            }
        }
    }

    void Domain::apply_pml_matrices() {
        assert(number_of_neighbours(false) == 1 and is_pml or number_of_neighbours(true) <= 2 and
               is_secondary_pml);
        // The pressure and velocity matrices are multiplied by the PML values.
        current_values.px0 *= pml_arrays.px;
        current_values.py0 *= pml_arrays.py;
        current_values.u0 *= pml_arrays.u;
        current_values.w0 *= pml_arrays.w;
    }


    void Domain::push_values() {
        previous_values = current_values;
    }


    int Domain::get_num_pmls_in_direction(Direction direction) {
        int num_pml_doms = 0;
        for (auto domain: get_neighbours_at(direction)) {
            if (domain->is_pml) {
                num_pml_doms++;
            }
        }
        return num_pml_doms;
    }

    void Domain::create_attenuation_array(CalcDirection calc_dir, bool ascending, ArrayXXf &pml_pressure,
                                          ArrayXXf &pml_velocity) {
        /*
         * 0mar: Most of this method only needs to be computed once for all domains.
         * However, the computations are not that big and only executed in the initialization phase.
         */

        //Pressure defined in cell centers
        auto pressure_range =
                VectorXf::LinSpaced(settings->GetPMLCells(), 0.5, float(settings->GetPMLCells() - 0.5)).array() /
                settings->GetPMLCells();
        //Velocity defined in cell edges
        auto velocity_range =
                VectorXf::LinSpaced(settings->GetPMLCells() + 1, 0, float(settings->GetPMLCells())).array() /
                settings->GetPMLCells();
        ArrayXXf alpha_pml_pressure = settings->GetAttenuationOfPMLCells() * pressure_range.pow(4);
        ArrayXXf alpha_pml_velocity =
                settings->GetDensityOfAir() * settings->GetAttenuationOfPMLCells() * velocity_range.pow(4);
        ArrayXXf pressure_pml_factors = (-alpha_pml_pressure * settings->GetTimeStep() /
                                         settings->GetDensityOfAir()).exp();
        ArrayXXf velocity_pml_factors = (-alpha_pml_velocity * settings->GetTimeStep()).exp();
        if (!ascending) {
            //Reverse if the attenuation takes place in the other direction
            pressure_pml_factors.reverseInPlace();
            velocity_pml_factors.reverseInPlace();
        }
        switch (calc_dir) {
            //Replicate matrices to size of domain
            case CalcDirection::X:
                pml_pressure = pressure_pml_factors.transpose().replicate(1, size.x);
                pml_velocity = velocity_pml_factors.transpose().replicate(1, size.x);
                break;
            case CalcDirection::Y:
                pml_pressure = pressure_pml_factors.replicate(size.y, 1);
                pml_velocity = velocity_pml_factors.replicate(size.y, 1);
                break;
        }
    }

    ostream &operator<<(ostream &str, Domain const &v) {
        string sort = v.id;
        if (v.is_pml) {
            sort += " (pml)";
        }
        str << sort << ", top left " << v.top_left << ", bottom right " << v.bottom_right;
        return str;
    }

    void Domain::post_initialization() {
        compute_number_of_neighbours();
        find_update_directions();
    }
}