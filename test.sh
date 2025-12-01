#!/bin/env bash

set -e

clear

echo "Running tests..."

echo "Listing files in /app..."
bin/cfrds ls $RDS_HOST/app
echo "Listed files."

echo "Uploading test file..."
echo "some file content" > /tmp/testfile.txt
bin/cfrds put /tmp/testfile.txt $RDS_HOST/app/testfile.txt

echo "Downloading and displaying test file..."
bin/cfrds get $RDS_HOST/app/testfile.txt /tmp/testfile.txt
bin/cfrds cat $RDS_HOST/app/testfile.txt

echo "Moving test file..."
bin/cfrds mv $RDS_HOST/app/testfile.txt /app/testfile2.txt

echo "Listing files in /app..."
bin/cfrds ls $RDS_HOST/app
echo "Listed files."

echo 'Removing test files...'
bin/cfrds rm $RDS_HOST/app/testfile2.txt
echo 'Removed testfile.txt'

echo "Listing files in /app..."
bin/cfrds ls $RDS_HOST/app
echo "Listed files."

echo "Creating new directory /app/newdir..."
bin/cfrds mkdir $RDS_HOST/app/newdir
echo "Created directory /app/newdir"

echo "Removing directory /app/newdir..."
bin/cfrds rmdir $RDS_HOST/app/newdir
echo "Removed directory /app/newdir"

echo "Listing files in /app..."
bin/cfrds ls $RDS_HOST/app
echo "Listed files."

echo "Fetching cfroot..."
bin/cfrds cfroot $RDS_HOST/
echo "Fetched cfroot"

echo "Fetching dsninfo..."
bin/cfrds dsninfo $RDS_HOST/
echo "Fetched dsninfo"

echo "Fetching tableinfo..."
bin/cfrds tableinfo $RDS_HOST/$RDS_DSN
echo "Fetched tableinfo"

echo "Fetching columninfo..."
bin/cfrds columninfo $RDS_HOST/$RDS_DSN/$RDS_DSN_TABLE
echo "Fetched columninfo"

echo "Fetching primarykeys..."
bin/cfrds primarykeys $RDS_HOST/$RDS_DSN/$RDS_DSN_TABLE
echo "Fetched primarykeys"

echo "Fetching foreignkeys..."
bin/cfrds foreignkeys $RDS_HOST/$RDS_DSN/$RDS_DSN_TABLE
echo "Fetched foreignkeys"

echo "Fetching importedkeys..."
bin/cfrds importedkeys $RDS_HOST/$RDS_DSN/$RDS_DSN_TABLE
echo "Fetched importedkeys"

echo "Fetching exportedkeys..."
bin/cfrds exportedkeys $RDS_HOST/$RDS_DSN/$RDS_DSN_TABLE
echo "Fetched exportedkeys"

echo "Fetching sql..."
bin/cfrds sql $RDS_HOST/$RDS_DSN "SELECT 1"
echo "Fetched sql"

echo "Fetching sqlmetadata..."
bin/cfrds sqlmetadata $RDS_HOST/$RDS_DSN "SELECT 1"
echo "Fetched sqlmetadata"

echo "Fetching dbdescription..."
bin/cfrds dbdescription $RDS_HOST/$RDS_DSN
echo "Fetched dbdescription"

echo "Done..."
