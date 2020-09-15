#include "NTT.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include "general_functions.h"
#include "RNS.h"
#include "BigIntLibrary/BigIntegerLibrary.hh"
#include <fstream>
#include <iomanip>

using namespace std;

///////////////////////////////////////////////////////////////
// Print Parameters
// prints NTT parameters
///////////////////////////////////////////////////////////////

void NTT::printParameters() {
    cout << "Entered modulus:" << min_mod << endl; 
    cout << "Used modulus:" << modulus << endl;
    cout << "n:" << vec_length << endl;
    cout << "nth root of unity:" << w_n << endl;
    cout << "nth root inverse:" << w_n_inv << endl << endl;


}

///////////////////////////////////////////////////////////////////////////////
//Create constant to vector
///////////////////////////////////////////////////////////////////////////////
vector<BigUnsigned> NTT::constant_vector(BigUnsigned length, BigUnsigned val) {
    vector<BigUnsigned> Z;
    for (BigUnsigned i = 0; i < length; i++) {
        Z.push_back(val);
    }
    return Z;
}

/////////////////////////////////////////////////////////////////
// Multiply vector by powers of val
///////////////////////////////////////////////////////////////
vector<BigUnsigned> NTT::mult_by_power(vector<BigUnsigned> vec, BigUnsigned val, BigUnsigned modulus) {
    for (int i = 0; i < vec.size(); i++) {
        vec[i] = (vec[i] * pow_mod(val, i, modulus)) % modulus;
    }

    return vec;
}
/////////////////////////////////////////////////////////////////
// generate table holding powers of phi
///////////////////////////////////////////////////////////////
vector<BigUnsigned> NTT::generate_phi_table(BigUnsigned n, BigUnsigned phi, BigUnsigned modulus) {
    vector<BigUnsigned> table;

    for (BigUnsigned i = 0; i < n / 2; i++) {
        table.push_back(pow_mod(phi, i, modulus));
    }

    return table;//bitReverse(table);
}
////////////////////////////////////////////////////////////////////////////////
// Checks and possibly readjusts modulus given min modulus M and vector length n
////////////////////////////////////////////////////////////////////////////////
BigUnsigned NTT::new_modulus(BigUnsigned vec_length, BigUnsigned min_modulus) {

    BigUnsigned k = (min_modulus - 1) / vec_length;  // should be floored,,, starting value to start the new_modulus off close to >= min_modulus
    BigUnsigned new_modulus = k * vec_length + 1;

    //increment k until modulus is prime and greater than the minimum modulus
    while ((new_modulus < min_modulus) || (factorize(new_modulus).size() > 1)) {
        k++;
        new_modulus = k * vec_length + 1;
    }

    return new_modulus;
}

///////////////////////////////////////////////////////////////
// Tests to find the nth root of unity
///////////////////////////////////////////////////////////////
BigUnsigned NTT::find_root_of_unity(BigUnsigned vec_length, BigUnsigned modulus) {
    //Get factors of n (vec_length)
    vector<BigUnsigned> factors = factorize(vec_length);

    //remove reaccuring factors
    sort(factors.begin(), factors.end());
    factors.erase(unique(factors.begin(), factors.end()), factors.end());


    for (BigUnsigned a = 1; a <= modulus; a++) {
        //test 1
        bool flag1 = (pow_mod(a, vec_length, modulus) == 1);

        //test 2: try for each prime factor of n
        bool flag2 = all_of(factors.begin(), factors.end(), [&](BigUnsigned prime) {return pow_mod(a, vec_length / prime, modulus) != 1; });

        if (flag2 && flag1)
            return a;
    }
 
    //error
    return -1;
}

///////////////////////////////////////////////////////////////
// Find generator
///////////////////////////////////////////////////////////////
bool NTT::is_generator(BigUnsigned val, BigUnsigned totient, BigUnsigned mod) {
    vector<BigUnsigned> factors = factorize(totient);

    //test 1
    bool flag1 = (pow_mod(val, totient, mod) == 1);

    //test 2: try for each prime factor of n
    bool flag2 = all_of(factors.begin(), factors.end(), [&](BigUnsigned prime) {return pow_mod(val, totient / prime, mod) != 1; });
  
    return (flag1 && flag2);
}

BigUnsigned NTT::find_generator(BigUnsigned totient, BigUnsigned mod) {
    for (BigUnsigned i = 1; i < mod; i++) {
        if (is_generator(i, totient, mod))
            return i;
    }
}
///////////////////////////////////////////////////////////////
// Tests to find the nth root of unity with generator
///////////////////////////////////////////////////////////////
    BigUnsigned NTT::find_root_of_unity2(BigUnsigned degree, BigUnsigned mod) {
    BigUnsigned totient = mod - 1;
    BigUnsigned generator = find_generator(totient, mod);
    BigUnsigned root = pow_mod(generator, totient / degree, mod);
    return root;
}

///////////////////////////////////////////////////////////////
// Solve all parameters
///////////////////////////////////////////////////////////////
vector<BigUnsigned> NTT::solveParameters(BigUnsigned vector_length, BigUnsigned minimum_modulus, bool modulusIsPrimeIPromise) {
    BigUnsigned modulus_local = minimum_modulus;
    if (modulusIsPrimeIPromise == false)
        modulus_local = NTT::new_modulus(vector_length, minimum_modulus);     // Used modulus
    BigUnsigned w_n_local = find_root_of_unity2(vector_length, modulus_local);        // nth root of unity
    BigUnsigned w_n_inv_local = mod_inverse(w_n_local, modulus_local);                         // root inverse
    BigUnsigned phi_local = sqrt_mod(w_n_local, modulus_local);
    BigUnsigned phi_inv_local = mod_inverse(phi_local, modulus_local);

    return vector<BigUnsigned> {modulus_local, w_n_local, w_n_inv_local, phi_local, phi_inv_local};
}
///////////////////////////////////////////////////////////////////////////////
// radix-2 Butterfly
///////////////////////////////////////////////////////////////////////////////
void NTT::butterfly(BigUnsigned* left_val, BigUnsigned* right_val) {
    *left_val  = (*left_val + *right_val) % modulus;
    *right_val = (*left_val + modulus - *right_val) % modulus;
}

///////////////////////////////////////////////////////////////////////////////
// Reference NTT (inefficient)
///////////////////////////////////////////////////////////////////////////////
    vector<BigUnsigned> NTT::stupidcalculate(vector<BigUnsigned> A, bool inverse) {;
    vector<BigUnsigned> Z; //return polynomial
    
    int vec_len = A.size();  //This should be same as NTT.vec_length but just in case
    
    BigUnsigned omeg = w_n;

    if (inverse) {
        omeg = w_n_inv;
    }

    for (int i = 0; i < vec_len; i++) {
        BigUnsigned val = 0;
        for (int j = 0; j < vec_len; j++) {
            val += A[j] * pow_mod(omeg, i * j, modulus);
        }
        Z.push_back(val % modulus);
        cout << i * 100 / vec_len << "% of studid NTT done.\r";
    }
    cout << "100% of studid NTT done." << endl;

    if (inverse) {
        Z = hadamard_product(Z, constant_vector(vec_len, mod_inverse(vec_len,modulus)), modulus);
    }

    return Z;
}






///////////////////////////////////////////////////////////////////////////////
// NTT 
// Based on Nayuki radix2, with RNS 

// The function rns.mult(a,b) uses the chosen RNS to multiply. That RNS
// uses montgomery reduction internally as long as the rns modulus is odd.
///////////////////////////////////////////////////////////////////////////////
vector<BigUnsigned> NTT::calculate(vector<BigUnsigned> A, bool inverse) {
    int n = A.size();
    int levels = log2(n);

    vector<BigUnsigned> powtable;

    BigUnsigned temp = 1;
    BigUnsigned omeg = w_n;
    if (inverse)
        omeg = w_n_inv;

    for (int i = 0; i < n / 2; i++) {
        powtable.push_back(temp);
        temp = rns.mult(temp, omeg) % modulus; //temp * omeg % modulus;
    }

    printVector(powtable);
    A = bitReverse(A);

    int size = 2;
    int count = 0;

    while (size <= n) {
        int halfsize = size / 2;
        int tablestep = n / size;

        for (int i = 0; i < n; i += size) {
            int  k = 0;
            for (int start = i; start < i + halfsize; start++) {
                int end = start + halfsize;
                BigUnsigned left = A[start];
                cout << "addr1: " << start << " addr2: " << end << endl;
                cout << "in1: " << A[start] << "   in2: " << A[end] << "      " << "  phi: " << powtable[k] << endl;
                BigUnsigned right = rns.mult(A[end], powtable[k]) % modulus; //(A[l] * powtable[k]) % modulus;
                A[start] = (left + right) % modulus;
                A[end] = (left + modulus - right) % modulus;
                cout << "out1: " << A[start] << "   out2: " << A[end] << endl << endl;
                k += tablestep;
            }
            count++;
         //cout << count*100/n << "% of butterfly NTT done.\r";
        }
        
        size += size;  // size = size * 2
    }
    

    //renamed NTT (not working)
    /*
    int vec_length = A.size();
    for (int block_height = 2; block_height < vec_length; block_height *= 2) {
        int n_blocks = n / block_height;
        int half_size = block_height / 2;
        for (int block = 0; block < n_blocks; block++) {
            int table_index = 0;
            int block_start = block * block_height;
            int block_end   = block * block_height + block_height;
            for (int start = block_start; start < block_end / 2; start++) {
                int end = start + half_size;
                BigUnsigned left = A[start];
                BigUnsigned right = rns.mult(A[end], powtable[table_index]) % modulus; //(A[l] * powtable[k]) % modulus;
                A[start] = (left + right) % modulus;
                A[end] = (left + modulus - right) % modulus;
                table_index += n_blocks;

               // butterfly(&left,&right);
            }
        }
    }
    */
    cout << "100% of butterfly NTT done." << endl;
    if (inverse)
        A = hadamard_product(A, constant_vector(n, mod_inverse(n, modulus)), modulus);
    return A;
}


/////////////////////////////////////////////////////////////////////////////
// Tests forw/inv for reference NTT and custom NTT on vector A
/////////////////////////////////////////////////////////////////////////////
void NTT::NTT_test(vector<BigUnsigned> A) {
    vector<BigUnsigned> Z;

    cout << endl << endl << "NTT TESTS:" << endl;
    printVector(A, "input:");
    cout << endl;

    //reference NTT
    Z = stupidcalculate(A);
    printVector(Z, "Stupid NTT:");
    Z = stupidcalculate(Z, true);
    printVector(Z, "result:");
    cout << endl;

    // efficient NTT 5
    Z = calculate(A);
    printVector(Z, "Nayuki NTT with RNS:");
    Z = calculate(Z, true);
    printVector(Z, "result:");
    cout << endl;
}
///////////////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////////////
NTT::NTT(BigUnsigned vector_length, BigUnsigned minimum_modulus, RNS rns_system, bool modulusIsPrimeIPromise) {
    vec_length = vector_length;
    min_mod = minimum_modulus;
    rns = rns_system;

    vector<BigUnsigned> params = solveParameters(vector_length, minimum_modulus, modulusIsPrimeIPromise);
    modulus = params[0];
    w_n     = params[1];
    w_n_inv = params[2];
    phi     = params[3];
    phi_inv = params[4];

    phi_table  = generate_phi_table(vector_length, phi, modulus);    // bit reversed powers of phi
}

///////////////////////////////////////////////////////////////////////////////
// Non-class functions for Vivado
///////////////////////////////////////////////////////////////////////////////

void save_twiddle_table(char* savename, BigUnsigned NTT_size, BigUnsigned w_n, BigUnsigned mod) {
    BigUnsigned temp = 1;
    ofstream Twiddlefile;
    Twiddlefile.open(savename);

    
    int hexwidth = mod.bitLength() / 4 + 2;

    for (BigUnsigned i = 0; i < NTT_size / 2; i++) {
        Twiddlefile << hex << setfill('0') << setw(hexwidth) << temp << "\n";
        temp = (temp * w_n) % mod; //temp * omeg % modulus;
    }
    Twiddlefile.close();
}


/*
//creates long text files with n polynomials A,B, and mult result C for FPGA testing 
void massPolynomialMultiply(int n_tests, BigUnsigned length, BigUnsigned minimum_modulus, vector<BigUnsigned> rns_moduli) {

    vector<BigUnsigned> A_ntt, B_ntt, C_ntt, C, polynomial_A, polynomial_B;

    //create RNS and NTT systems
    RNS rns_system(rns_moduli);
    NTT ntt_system(length, minimum_modulus, rns_system);

    for (int i = 0; i < n_tests; i++) {
        //Random polynomials
        polynomial_A = sample_polynomial(length, minimum_modulus);
        polynomial_B = sample_polynomial(length, minimum_modulus);

        //Butterfly NTT with RNS
        A_ntt = ntt_system.calculate(polynomial_A);
        B_ntt = ntt_system.calculate(polynomial_B);
        C_ntt = hadamard_product(A_ntt, B_ntt, ntt_system.modulus);
        C = ntt_system.calculate(C_ntt, true);
    }
}

void massNTT(int n_tests, BigUnsigned length,BigUnsigned minimum_modulus, vector<BigUnsigned> rns_moduli){

    vector<BigUnsigned> A, A_ntt;

    //create RNS and NTT systems
    RNS rns_system(rns_moduli);
    NTT ntt_system(length, minimum_modulus, rns_system);


    for (int i = 0; i < n_tests; i++) {
        ofstream Afile,NTTfile;

        cout << i << "\n";
        //Random polynomials
        A = sample_polynomial(length, minimum_modulus);

        //save polynomial
        Afile.open("A" + to_string(i) + ".txt");
        for (int j = 0; j < A.size(); j++) {
            Afile << hex << A[j] << "\n";
        }
        Afile.close();

        //Butterfly NTT with RNS
        A_ntt = ntt_system.calculate(A);

        //save NTT
        NTTfile.open("A_NTT" + to_string(i) + ".txt");
        for (int j = 0; j < A_ntt.size(); j++) {
            NTTfile << hex << A_ntt[j] << "\n";
        }
        NTTfile.close();
    }
}

*/