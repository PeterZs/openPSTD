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
// Date: 2-1-2016
//
//
// Authors: Omar Richardson
//
//
// Purpose: Test suite for kernel functions
//
//
//////////////////////////////////////////////////////////////////////////


#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE Main
#endif

#include <boost/test/unit_test.hpp>
#include "../../kernel/core/kernel_functions.h"
#include <kernel/PSTDKernel.h>
#include <cmath>

using namespace OpenPSTD::Kernel;
using namespace std;
using namespace Eigen;
BOOST_AUTO_TEST_SUITE(kernel_functions)

    BOOST_AUTO_TEST_CASE(test_next_2_power) {
        BOOST_CHECK_EQUAL(next_2_power(42), 64);
        BOOST_CHECK_EQUAL(next_2_power(3.4), 4);
        BOOST_CHECK_EQUAL(next_2_power(0.1), 1);
    }

    BOOST_AUTO_TEST_CASE(test_rho_array_one_neighbour) {
        float air_dens = 1.2;
        float max_rho = 1E10;
        Array<float, 4, 2> velocity;
        Array<float, 4, 2> pressure;
        velocity << -1, 1, 0, 0, 2, 0, 1, 1;
        pressure << 1, -1, 0, 0, 2, 0, 1, 1;
        RhoArray rhoArray = get_rho_array(max_rho, air_dens, air_dens);
        BOOST_CHECK(rhoArray.pressure.isApprox(pressure));
        BOOST_CHECK(rhoArray.velocity.isApprox(velocity));
    }

    BOOST_AUTO_TEST_CASE(test_rho_array_two_neighbour) {
        float air_dens = 1.2;
        Array<float, 4, 2> velocity;
        Array<float, 4, 2> pressure;
        velocity << 0, 0, 0, 0, 1, 1, 1, 1;
        pressure << 0, 0, 0, 0, 1, 1, 1, 1;
        RhoArray rhoArray = get_rho_array(air_dens, air_dens, air_dens);
        BOOST_CHECK(rhoArray.pressure.isApprox(pressure));
        BOOST_CHECK(rhoArray.velocity.isApprox(velocity));
    }

    BOOST_AUTO_TEST_CASE(test_get_grid_spacing) {
        PSTDSettings settings;
        settings.SetSoundSpeed(340);
        settings.SetMaxFrequency(84000);
        BOOST_CHECK(get_grid_spacing(settings) <= 0.0020238);
        settings.SetSoundSpeed(340);
        settings.SetMaxFrequency(20000);
        BOOST_CHECK(get_grid_spacing(settings) <= 0.0085);
        settings.SetSoundSpeed(340);
        settings.SetMaxFrequency(200);
        BOOST_CHECK(get_grid_spacing(settings) <= 0.85);
        settings.SetSoundSpeed(340);
        settings.SetMaxFrequency(20);
        BOOST_CHECK(get_grid_spacing(settings) <= 8.5);
    }

    BOOST_AUTO_TEST_CASE(test_spatderp3) {
        Eigen::ArrayXXf d1(4,50), d2(4,50), d3(4,50);
        Eigen::ArrayXcf derfact(128);
        Eigen::ArrayXf real(128), imag(128);

        d1 = Eigen::ArrayXXf::Zero(4,50);
        d3 = Eigen::ArrayXXf::Zero(4,50);

        d2 << 1.61399787e-207,4.56817714e-207,6.46184092e-207,4.56817714e-207, 1.61399787e-207,2.84994399e-208,2.51502926e-209,1.10923382e-210, 2.44498678e-212,2.69341499e-214,1.48287102e-216,4.08015658e-219, 5.61078503e-222,3.85605995e-225,1.32445520e-228,2.27354793e-232, 1.95049258e-236,8.36291802e-241,1.79202772e-245,1.91913248e-250, 1.02716103e-255,2.74754908e-261,3.67304063e-267,2.45402709e-273, 8.19419550e-280,1.36743483e-286,1.14046064e-293,4.75365118e-301, 9.90256421e-309,1.03095893e-316,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,
                1.59468819e-196,4.51352402e-196,6.38453224e-196,4.51352402e-196, 1.59468819e-196,2.81584761e-197,2.48493975e-198,1.09596308e-199, 2.41573525e-201,2.66119129e-203,1.46513013e-205,4.03134208e-208, 5.54365828e-211,3.80992651e-214,1.30860957e-217,2.24634747e-221, 1.92715712e-225,8.26286507e-230,1.77058812e-234,1.89617221e-239, 1.01487220e-244,2.71467774e-250,3.62909682e-256,2.42466741e-262, 8.09616114e-269,1.35107500e-275,1.12681630e-282,4.69677906e-290, 9.78409113e-298,1.01862468e-305,5.30006561e-314,1.38338381e-322, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,
                7.87448115e-186,2.22875293e-185,3.15264633e-185,2.22875293e-185, 7.87448115e-186,1.39044981e-186,1.22704936e-187,5.41180444e-189, 1.19287656e-190,1.31408138e-192,7.23473067e-195,1.99065418e-197, 2.73742747e-200,1.88132041e-203,6.46184092e-207,1.10923382e-210, 9.51619414e-215,4.08015658e-219,8.74306514e-224,9.36319235e-229, 5.01138218e-234,1.34049269e-239,1.79202772e-245,1.19728721e-251, 3.99783911e-258,6.67153283e-265,5.56415591e-272,2.31924325e-279, 4.83132950e-287,5.02991172e-295,2.61714277e-303,6.80562186e-312, 8.84377506e-321,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,
                1.94330722e-175,5.50023750e-175,7.78027181e-175,5.50023750e-175, 1.94330722e-175,3.43142754e-176,3.02817903e-177,1.33555449e-178, 2.94384556e-180,3.24296140e-182,1.78542612e-184,4.91264449e-187, 6.75557216e-190,4.64282468e-193,1.59468819e-196,2.73742747e-200, 2.34845807e-204,1.00692320e-208,2.15766109e-213,2.31069945e-218, 1.23673611e-223,3.30814065e-229,4.42246332e-235,2.95473040e-241, 9.86608445e-248,1.64643710e-254,1.37315261e-261,5.72355443e-269, 1.19230173e-276,1.24130893e-284,6.45872705e-293,1.67952832e-301, 2.18273533e-310,1.41772137e-319,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000,0.00000000e+000,0.00000000e+000, 0.00000000e+000,0.00000000e+000;

        real << 0.00000000e+00, 3.01166185e-03, 1.20430192e-02, 2.70831905e-02, 4.81140512e-02, 7.51102496e-02, 1.08039230e-01, 1.46861262e-01, 1.91529476e-01, 2.41989907e-01, 2.98181542e-01, 3.60036376e-01, 4.27479473e-01, 5.00429034e-01, 5.78796470e-01, 6.62486486e-01, 7.51397162e-01, 8.45420051e-01, 9.44440271e-01, 1.04833662e+00, 1.15698166e+00, 1.27024188e+00, 1.38797777e+00, 1.51004397e+00, 1.63628940e+00, 1.76655741e+00, 1.90068588e+00, 2.03850743e+00, 2.17984953e+00, 2.32453465e+00, 2.47238048e+00, 2.62320003e+00, 2.77680184e+00, 2.93299013e+00, 3.09156503e+00, 3.25232270e+00, 3.41505557e+00, 3.57955249e+00, 3.74559896e+00, 3.91297732e+00, 4.08146692e+00, 4.25084437e+00, 4.42088374e+00, 4.59135674e+00, 4.76203297e+00, 4.93268010e+00, 5.10306413e+00, 5.27294956e+00, 5.44209966e+00, 5.61027667e+00, 5.77724203e+00, 5.94275659e+00, 6.10658086e+00, 6.26847523e+00, 6.42820020e+00, 6.58551662e+00, 6.74018588e+00, 6.89197020e+00, 7.04063282e+00, 7.18593822e+00, 7.32765241e+00, 7.46554307e+00, 7.59937988e+00, 7.72893466e+00, 7.85398163e+00, 7.72893466e+00, 7.59937988e+00, 7.46554307e+00, 7.32765241e+00, 7.18593822e+00, 7.04063282e+00, 6.89197020e+00, 6.74018588e+00, 6.58551662e+00, 6.42820020e+00, 6.26847523e+00, 6.10658086e+00, 5.94275659e+00, 5.77724203e+00, 5.61027667e+00, 5.44209966e+00, 5.27294956e+00, 5.10306413e+00, 4.93268010e+00, 4.76203297e+00, 4.59135674e+00, 4.42088374e+00, 4.25084437e+00, 4.08146692e+00, 3.91297732e+00, 3.74559896e+00, 3.57955249e+00, 3.41505557e+00, 3.25232270e+00, 3.09156503e+00, 2.93299013e+00, 2.77680184e+00, 2.62320003e+00, 2.47238048e+00, 2.32453465e+00, 2.17984953e+00, 2.03850743e+00, 1.90068588e+00, 1.76655741e+00, 1.63628940e+00, 1.51004397e+00, 1.38797777e+00, 1.27024188e+00, 1.15698166e+00, 1.04833662e+00, 9.44440271e-01, 8.45420051e-01, 7.51397162e-01, 6.62486486e-01, 5.78796470e-01, 5.00429034e-01, 4.27479473e-01, 3.60036376e-01, 2.98181542e-01, 2.41989907e-01, 1.91529476e-01, 1.46861262e-01, 1.08039230e-01, 7.51102496e-02, 4.81140512e-02, 2.70831905e-02, 1.20430192e-02, 3.01166185e-03;
        imag << +0.00000000e+00, 1.22681503e-01, +2.45141287e-01, 3.67157856e-01, +4.88510160e-01, 6.08977815e-01, +7.28341326e-01, 8.46382306e-01, +9.62883697e-01, 1.07762999e+00, +1.19040744e+00, 1.30100429e+00, +1.40921097e+00, 1.51482031e+00, +1.61762777e+00, 1.71743163e+00, +1.81403322e+00, 1.90723707e+00, +1.99685118e+00, 2.08268715e+00, +2.16456044e+00, 2.24229050e+00, +2.31570101e+00, 2.38462001e+00, +2.44888015e+00, 2.50831879e+00, +2.56277824e+00, 2.61210587e+00, +2.65615433e+00, 2.69478167e+00, +2.72785150e+00, 2.75523316e+00, +2.77680184e+00, 2.79243874e+00, +2.80203121e+00, 2.80547286e+00, +2.80266368e+00, 2.79351018e+00, +2.77792552e+00, 2.75582955e+00, +2.72714900e+00, 2.69181751e+00, +2.64977574e+00, 2.60097147e+00, +2.54535965e+00, 2.48290251e+00, +2.41356958e+00, 2.33733779e+00, +2.25419149e+00, 2.16412252e+00, +2.06713025e+00, 1.96322159e+00, +1.85241105e+00, 1.73472072e+00, +1.61018033e+00, 1.47882721e+00, +1.34070633e+00, 1.19587027e+00, +1.04437922e+00, 8.86300945e-01, +7.21710769e-01, 5.50691541e-01, +3.73333594e-01, 1.89734696e-01, +4.80917673e-16, 1.89734696e-01, -3.73333594e-01, 5.50691541e-01, -7.21710769e-01, 8.86300945e-01, -1.04437922e+00, 1.19587027e+00, -1.34070633e+00, 1.47882721e+00, -1.61018033e+00, 1.73472072e+00, -1.85241105e+00, 1.96322159e+00, -2.06713025e+00, 2.16412252e+00, -2.25419149e+00, 2.33733779e+00, -2.41356958e+00, 2.48290251e+00, -2.54535965e+00, 2.60097147e+00, -2.64977574e+00, 2.69181751e+00, -2.72714900e+00, 2.75582955e+00, -2.77792552e+00, 2.79351018e+00, -2.80266368e+00, 2.80547286e+00, -2.80203121e+00, 2.79243874e+00, -2.77680184e+00, 2.75523316e+00, -2.72785150e+00, 2.69478167e+00, -2.65615433e+00, 2.61210587e+00, -2.56277824e+00, 2.50831879e+00, -2.44888015e+00, 2.38462001e+00, -2.31570101e+00, 2.24229050e+00, -2.16456044e+00, 2.08268715e+00, -1.99685118e+00, 1.90723707e+00, -1.81403322e+00, 1.71743163e+00, -1.61762777e+00, 1.51482031e+00, -1.40921097e+00, 1.30100429e+00, -1.19040744e+00, 1.07762999e+00, -9.62883697e-01, 8.46382306e-01, -7.28341326e-01, 6.08977815e-01, -4.88510160e-01, 3.67157856e-01, -2.45141287e-01, 1.22681503e-01;

        derfact.real() = real;
        derfact.imag() = imag;

        int wlen = 32;
        Eigen::ArrayXf window(65);
        window << 0.00316228,0.00858261,0.02007542,0.0412163 ,0.07551126,0.12530442,0.19087516,0.27012564,0.35896633,0.45219639,0.54452377,0.63140816,0.7095588 ,0.77707471,0.83331485,0.8786185 ,0.9139817 ,0.94076063,0.96043711,0.97445482,0.98411922,0.9905474 ,0.99465322,0.99715493,0.9985956 ,0.99936947,0.9997499 ,0.99991624,0.99997804,0.99999609,0.99999966,0.99999999,1.        ,0.99999999,0.99999966,0.99999609,0.99997804,0.99991624,0.9997499 ,0.99936947,0.9985956 ,0.99715493,0.99465322,0.9905474 ,0.98411922,0.97445482,0.96043711,0.94076063,0.9139817 ,0.8786185 ,0.83331485,0.77707471,0.7095588 ,0.63140816,0.54452377,0.45219639,0.35896633,0.27012564,0.19087516,0.12530442,0.07551126,0.0412163 ,0.02007542,0.00858261,0.00316228;

        RhoArray rho_array;
        ArrayXXf pres_rhos(4,2);
        pres_rhos << 1, -1,
                     0, 0,
                     2, 2.40000009537e-200,
                     1, 1;
        rho_array.pressure = pres_rhos;

        //Eigen::ArrayXXf spatresult = spatderp3(d1, d2, d3, derfact, rho_array, window, wlen,
        //                          CalculationType::PRESSURE, CalcDirection::X);

        Eigen::ArrayXXf spatexpectation(4,51);
        spatexpectation <<  -1.12252533e-214, 7.87226795e-207, 5.10169599e-207,-5.12451121e-207, -7.70826560e-207,-3.20302239e-207,-5.62315765e-208,-4.37555445e-209, -2.45182627e-210, 4.39335069e-211,-3.09640710e-211, 2.09824295e-211, -1.46786195e-211, 1.05675619e-211,-7.80819768e-212, 5.90500040e-212, -4.55893917e-212, 3.58487366e-212,-2.86522105e-212, 2.32345980e-212, -1.90863528e-212, 1.58608581e-212,-1.33176296e-212, 1.12867511e-212, -9.64613364e-213, 8.30668311e-213,-7.20244691e-213, 6.28395277e-213, -5.51363334e-213, 4.86263524e-213,-4.30856250e-213, 3.83385947e-213, -3.42463787e-213, 3.06981583e-213,-2.76047955e-213, 2.48940465e-213, -2.25069434e-213, 2.03950300e-213,-1.85182353e-213, 1.68432222e-213, -1.53420974e-213, 1.39913936e-213,-1.27712649e-213, 1.16648418e-213, -1.06577155e-213, 9.73751967e-214,-8.89359187e-214, 8.11669551e-214, -7.39879285e-214, 6.73285634e-214,-6.11271299e-214,
                            -1.10909558e-203, 7.77808508e-196, 5.04065991e-196,-5.06320217e-196, -7.61604484e-196,-3.16470181e-196,-5.55588287e-197,-4.32320584e-198, -2.42249292e-199, 4.34078917e-200,-3.05936206e-200, 2.07313983e-200, -1.45030063e-200, 1.04411329e-200,-7.71478135e-201, 5.83435368e-201, -4.50439657e-201, 3.54198466e-201,-2.83094189e-201, 2.29566221e-201, -1.88580059e-201, 1.56711007e-201,-1.31582991e-201, 1.11517178e-201, -9.53072845e-202, 8.20730295e-202,-7.11627771e-202, 6.20877232e-202, -5.44766890e-202, 4.80445925e-202,-4.25701538e-202, 3.78799164e-202, -3.38366592e-202, 3.03308893e-202,-2.72745351e-202, 2.45962171e-202, -2.22376730e-202, 2.01510263e-202,-1.82966854e-202, 1.66417119e-202, -1.51585464e-202, 1.38240023e-202,-1.26184710e-202, 1.15252850e-202, -1.05302078e-202, 9.62102115e-203,-8.78719001e-203, 8.01958835e-203, -7.31027459e-203, 6.65230527e-203,-6.03958124e-203,
                            -5.47665199e-193, 3.84077493e-185, 2.48904968e-185,-2.50018093e-185, -3.76076037e-185,-1.56271206e-185,-2.74346390e-186,-2.13477488e-187, -1.19621346e-188, 2.14345743e-189,-1.51069588e-189, 1.02370486e-189, -7.16150341e-190, 5.15577305e-190,-3.80951590e-190, 2.88097123e-190, -2.22424584e-190, 1.74901223e-190,-1.39790328e-190, 1.13358516e-190, -9.31197791e-191, 7.73830190e-191,-6.49749452e-191, 5.50665591e-191, -4.70622043e-191, 4.05272032e-191,-3.51397815e-191, 3.06585706e-191, -2.69002845e-191, 2.37241512e-191,-2.10209040e-191, 1.87048910e-191, -1.67083531e-191, 1.49772236e-191,-1.34680129e-191, 1.21454745e-191, -1.09808386e-191, 9.95046413e-192,-9.03480095e-192, 8.21758432e-192, -7.48520544e-192, 6.82621504e-192,-6.23093046e-192, 5.69112131e-192, -5.19975777e-192, 4.75080646e-192,-4.33906530e-192, 3.96002790e-192, -3.60977273e-192, 3.28487115e-192,-2.98231145e-192,
                            -1.35155791e-182, 9.47847295e-175, 6.14261198e-175,-6.17008226e-175, -9.28100865e-175,-3.85654569e-175,-6.77046920e-176,-5.26831336e-177, -2.95208053e-178, 5.28974064e-179,-3.72817734e-179, 2.52635444e-179, -1.76735470e-179, 1.27236967e-179,-9.40133021e-180, 7.10981726e-180, -5.48911468e-180, 4.31630735e-180,-3.44982162e-180, 2.79752303e-180, -2.29806048e-180, 1.90970016e-180,-1.60348698e-180, 1.35896245e-180, -1.16142663e-180, 1.00015233e-180,-8.67198610e-181, 7.56608854e-181, -6.63859828e-181, 5.85477487e-181,-5.18765284e-181, 4.61609457e-181, -4.12337812e-181, 3.69616056e-181,-3.32370937e-181, 2.99732615e-181, -2.70991099e-181, 2.45562957e-181,-2.22965724e-181, 2.02798008e-181, -1.84723965e-181, 1.68461042e-181,-1.53770286e-181, 1.40448582e-181, -1.28322446e-181, 1.17242982e-181,-1.07081810e-181, 9.77277189e-182, -8.90839317e-182, 8.10658342e-182,-7.35991015e-182;
        BOOST_CHECK(true);
    }


BOOST_AUTO_TEST_SUITE_END()
