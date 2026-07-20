#!/usr/bin/env bash

set -e

echo "Running C CLI (cfrds) tests..."

USER="${RDS_USERNAME:-admin}"
PASS="${RDS_PASSWORD:-admin}"
HOST="${RDS_HOST:-192.168.100.10}"
PORT="${RDS_PORT:-8500}"

TARGET="rds://${USER}:${PASS}@${HOST}:${PORT}"
echo "Target server: ${TARGET}"

# Pre-cleanup
bin/cfrds rm "${TARGET}/tmp/c_testfile.txt" 2>/dev/null || true
bin/cfrds rm "${TARGET}/tmp/c_testfile2.txt" 2>/dev/null || true
bin/cfrds rmdir "${TARGET}/tmp/c_test_dir" 2>/dev/null || true

echo "1. Testing file operations..."
echo "Creating temp directory..."
bin/cfrds mkdir "${TARGET}/tmp/c_test_dir"
echo "Directory created."

echo "Uploading test file..."
echo "some file content from C test" > /tmp/c_testfile.txt
bin/cfrds put /tmp/c_testfile.txt "${TARGET}/tmp/c_testfile.txt"

echo "Downloading and displaying test file..."
bin/cfrds get "${TARGET}/tmp/c_testfile.txt" /tmp/c_testfile_out.txt
bin/cfrds cat "${TARGET}/tmp/c_testfile.txt"

echo "Moving test file..."
bin/cfrds mv "${TARGET}/tmp/c_testfile.txt" "/tmp/c_testfile2.txt"

echo "Listing files in /tmp..."
bin/cfrds ls "${TARGET}/tmp"

echo "Removing test file..."
bin/cfrds rm "${TARGET}/tmp/c_testfile2.txt"

echo "Removing directory..."
bin/cfrds rmdir "${TARGET}/tmp/c_test_dir"

echo "2. Testing root directory..."
bin/cfrds cfroot "${TARGET}/"

echo "3. Testing SQL operations..."
echo "Fetching supportedcommands..."
bin/cfrds supportedcommands "${TARGET}/"

echo "Fetching dsninfo..."
DSN_LIST=$(bin/cfrds dsninfo "${TARGET}/" | tr '\n' ' ')
echo "Fetched dsninfo: ${DSN_LIST}"

DSN="${RDS_DSN:-}"
if [ -z "$DSN" ]; then
    DSN=$(echo "$DSN_LIST" | awk '{print $1}')
fi

if [ -n "$DSN" ]; then
    echo "Using DSN: ${DSN}"
    
    echo "Fetching dbdescription..."
    bin/cfrds dbdescription "${TARGET}/${DSN}" || true

    echo "Fetching tableinfo..."
    TABLE_LIST=$(bin/cfrds tableinfo "${TARGET}/${DSN}" | head -n 1 | awk '{print $3}')
    echo "Fetched tableinfo."

    DSN_TABLE="${RDS_DSN_TABLE:-}"
    if [ -z "$DSN_TABLE" ]; then
        DSN_TABLE="$TABLE_LIST"
    fi

    if [ -n "$DSN_TABLE" ]; then
        echo "Using table: ${DSN_TABLE}"
        bin/cfrds columninfo "${TARGET}/${DSN}/${DSN_TABLE}" || true
        bin/cfrds primarykeys "${TARGET}/${DSN}/${DSN_TABLE}" || true
        bin/cfrds foreignkeys "${TARGET}/${DSN}/${DSN_TABLE}" || true
        bin/cfrds importedkeys "${TARGET}/${DSN}/${DSN_TABLE}" || true
        bin/cfrds exportedkeys "${TARGET}/${DSN}/${DSN_TABLE}" || true
    fi

    echo "Fetching sql..."
    bin/cfrds sql "${TARGET}/${DSN}" "SELECT 1" || true

    echo "Fetching sqlmetadata..."
    bin/cfrds sqlmetadata "${TARGET}/${DSN}" "SELECT 1" || true
else
    echo "No DSN available, skipping DSN-specific SQL tests."
fi

echo "4. Testing Security Analyzer..."
bin/cfrds security_analyzer "${TARGET}/app" || true

echo "5. Testing IDE Default..."
bin/cfrds ide_default "${TARGET}/" 1 || true

echo "6. Debugger Operations..."
# TODO: Test debugger operations (skipped for now)
echo "Skipping debugger tests (TODO)"

echo "C CLI (cfrds) tests completed successfully!"
