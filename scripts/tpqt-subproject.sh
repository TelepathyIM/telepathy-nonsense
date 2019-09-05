#!/bin/sh

# set -x

SCRIPT=$(readlink -f $0)
SCRIPTPATH=$(dirname $SCRIPT)
cd $SCRIPTPATH/..
SOURCE_PROJECT_DIR=`pwd`

BASE_DIR="$SOURCE_PROJECT_DIR/.."

# setup
PROJECT_NAME="nonsense"

# Build dir variable names itself
if [ x"$TELEPATHY_QT_DIR" = "x" ]; then
    TELEPATHY_QT_DIR="$BASE_DIR/telepathy-qt"
fi

TARGET_EXAMPLES_DIR=$TELEPATHY_QT_DIR/examples
TARGET_PROJECT_DIR=$TARGET_EXAMPLES_DIR/$PROJECT_NAME

echo "TelepathyQt dir: $TELEPATHY_QT_DIR"
echo "Project source dir: $SOURCE_PROJECT_DIR"
echo "Project target dir: $TARGET_PROJECT_DIR"

# Link the project source files to the target example dir
if [ -d "$TARGET_PROJECT_DIR" ]; then
  echo "$TARGET_PROJECT_DIR already exists. Say 'y' to remove (and recreate)."
  rm -rI $TARGET_PROJECT_DIR
fi

mkdir -p $TARGET_PROJECT_DIR

cd $TARGET_PROJECT_DIR
ln -s $SOURCE_PROJECT_DIR/*.cc .
ln -s $SOURCE_PROJECT_DIR/*.hh .
ln -s $SOURCE_PROJECT_DIR/*.in .

# Register the project as an example
cd $TARGET_EXAMPLES_DIR

if [ `grep $PROJECT_NAME CMakeLists.txt|wc -l` = '0' ]; then
 echo "add_subdirectory($PROJECT_NAME)" >> CMakeLists.txt
fi

# Adjust the project CMake files
cd $TARGET_PROJECT_DIR
cp $SOURCE_PROJECT_DIR/CMakeLists.txt .

# Remove all find_package TelepathyQt
sed -i '/find_package(TelepathyQt/d' CMakeLists.txt

# Remove all find_package QXmpp
sed -i '/find_package(QXmpp/d' CMakeLists.txt

# ${QXMPP_INCLUDE_DIR} to qxmpp/src/base and qxmpp/src/client
sed -i '/${QXMPP_INCLUDE_DIR}/d' CMakeLists.txt

# Replace TelepathyQt CMake-found libraries with TpQt "this project" ones.
sed -i 's/\${TELEPATHY_QT._LIBRARIES}/telepathy-qt\${QT_VERSION_MAJOR}/g' CMakeLists.txt
sed -i 's/\${TELEPATHY_QT._SERVICE_LIBRARIES}/telepathy-qt\${QT_VERSION_MAJOR}-service/g' CMakeLists.txt

# Replace QXmpp CMake-found libraries with "this project" ones.
sed -i 's/QXmpp::QXmpp/qxmpp/g' CMakeLists.txt

# Link the backend library source files
mkdir -p $TARGET_PROJECT_DIR/qxmpp
ln -s $BASE_DIR/qxmpp/* $TARGET_PROJECT_DIR/qxmpp/
rm $TARGET_PROJECT_DIR/qxmpp/CMakeLists.txt
cp $BASE_DIR/qxmpp/CMakeLists.txt $TARGET_PROJECT_DIR/qxmpp/CMakeLists.txt

# Remove all add_subdirectory
sed -i '/add_subdirectory/d' $TARGET_PROJECT_DIR/qxmpp/CMakeLists.txt

echo "remove_definitions(-DQT_NO_CAST_FROM_ASCII)" >> $TARGET_PROJECT_DIR/qxmpp/CMakeLists.txt
# Add back the main (sources) subdir
echo "add_subdirectory(src)" >> $TARGET_PROJECT_DIR/qxmpp/CMakeLists.txt

# Plug QXmpp to the target project
echo "add_subdirectory(qxmpp)" >> $TARGET_PROJECT_DIR/CMakeLists.txt
