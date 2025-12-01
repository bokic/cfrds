# cfrds - ColdFusion RDS protocol library with python bindings and CLI interface.

## Project description
cfrds is a library and cli executable designed to communicate to ColdFusion server via Adobe RDS protocol. More info regarding the protocol at https://helpx.adobe.com/coldfusion/coldfusion-builder-extension-for-visual-studio-code/rds-support.html.

Project is written in C. It's portable and can be compiled for Windows, Linux, and MacOS, Intel and ARM 32/64bit targets.

Some of the features RDS protocol supports:
* Server side file access(list dir, upload, download file).
* Enumerating databases connections/tables/table structures(etc)
* Executing SQL on server side.
* Remote ColdFusion debugger.
* other...

## Implemented Features
* Remote ColdFusion server file/dir access.
* Remote ColdFusion server Database access.
* Remote ColdFusion server debugger.

## TODO
* Remote ColdFusion server adminapi settings.
* Remote ColdFusion server graph(chart).
* Remote ColdFusion server webapp security analyzer service.
* Code cleanup.

## CLI test script usage ex:
> RDS_HOST=rds://localhost:8500 RDS_DSN=cf_dsn RDS_DSN_TABLE=some_table ./test.sh

## CLI examples
* List directory - `cfrds ls <rds://{username}:{password}@{host}:{port}/{pathname}>`
* Print file content - `cfrds cat <rds://{username}:{password}@{host}:{port}/{pathname}>`
* File download - `cfrds download <rds://{username}:{password}@{host}:{port}/{pathname}> <destination_file>`
* File upload - `cfrds upload <source_file> <rds://{username}:{password}@{host}:{port}/{pathname}>`
* File rename - `cfrds mv <source_file> <rds://{username}:{password}@{host}:{port}/{pathname}> <destination_file>`
* File delete - `cfrds rm <source_file> <rds://{username}:{password}@{host}:{port}/{pathname}>`
* Create directory - `cfrds mkdir <rds://{username}:{password}@{host}:{port}/{pathname}>`
* Delete directory - `cfrds rmdir <rds://{username}:{password}@{host}:{port}/{pathname}>`
* Get ColdFusion installation directory - `cfrds cfroot <rds://{username}:{password}@{host}:{port}>`
* Get ColdFusion datasources info - `cfrds dsninfo <rds://{username}:{password}@{host}:{port}>`
* Get ColdFusion datasource tables info - `cfrds tableinfo <rds://{username}:{password}@{host}:{port}/{dsn}>`
* Get ColdFusion datasource table columns info - `cfrds columninfo <rds://{username}:{password}@{host}:{port}/{dsn}/{table}>`
* Get ColdFusion datasource table primary keys info - `cfrds primarykeys <rds://{username}:{password}@{host}:{port}/{dsn}/{table}>`
* Get ColdFusion datasource table foreign keys info - `cfrds primarykeys <rds://{username}:{password}@{host}:{port}/{dsn}/{table}>`
* Get ColdFusion datasource table imported keys info - `cfrds primarykeys <rds://{username}:{password}@{host}:{port}/{dsn}/{table}>`
* Get ColdFusion datasource table exported keys info - `cfrds primarykeys <rds://{username}:{password}@{host}:{port}/{dsn}/{table}>`
* Execute ColdFusion datasource SQL - `cfrds sql <rds://{username}:{password}@{host}:{port}/{dsn}> <SQL>`
* Get ColdFusion datasource SQL metadata - `cfrds sqlmetadata <rds://{username}:{password}@{host}:{port}/{dsn}> <SQL>`
* Get ColdFusion datasource database server info - `cfrds dbdescription <rds://{username}:{password}@{host}:{port}/{dsn}>`
* Start ColdFusion debugger - `cfrds dbg_start <rds://{username}:{password}@{host}:{port}>`
* Stop ColdFusion debugger - `cfrds dbg_stop <rds://{username}:{password}@{host}:{port}>/{session_id}`
* Get ColdFusion debugger server info - `cfrds dbg_info <rds://{username}:{password}@{host}:{port}>`

## Build
> cd \<root of the project\>
> 
> mkdir build
> 
> cmake -Bbuild -G Ninja
> 
> ninja -Cbuild

## Installation
* Linux
  * arch - `yay -S cfrds`
  * Ubuntu PPA - Work in progress.
* Windows
  * Windows installation - Work in progress.
* MacOS
  * homebrew - Work in progress.
