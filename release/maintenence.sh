#!/bin/bash

set -e

die() {
    echo >&2 "$@"
    exit 1
}

# check input
BASE=${CORE_DIR?"Environment variable CORE_DIR must be set to the cyclus repository directory."}
STUB=${STUB_DIR?"Environment variable STUB_DIR must be set to the cycstub repository directory."}
[ "$#" -eq 1 ] || die "Must provide the version (e.g., X.X.X) as an argument" 
VERSION=$1
echo "Performing maintence updates for Cyclus stack verison $VERSION"

# cyclus
cd $BASE/release
./smbchk.py --update -t HEAD --no-save --check | grep "ABI stability has been achieved!"
./smbchk.py --update -t $VERSION
cd $BASE

# cycstub
cp $BASE/tests/input/stub_example.xml $STUB/input/example.xml
cp $BASE/stubs/stub_* $STUB/src/

# pyne
git clone git@github.com:pyne/pyne
cd pyne
./amalgamate.py -s pyne.cc -i pyne.h
cp pyne.* $BASE/src
cd ..
rm -rf pyne

# nuc_data upload
cd $BASE/release
nuc_data_make -o cyclus_nuc_data.h5 \
    -m atomic_mass,scattering_lengths,decay,simple_xs,materials,eaf,wimsd_fpy,nds_fpy
python upload_nuc_data.py
rm -r build_nuc_data cyclus_nuc_data.h5
cd $BASE

echo "
*-----------------------------------------------------------------------------*
You're almost done!

Cyclus, Cycamore, and Cycstub release candidates still need to be committed and
pushed upstream

Once the candidate passes CI it should be ready to be merged into develop and
master.
*-----------------------------------------------------------------------------*
"
