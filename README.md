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

## Implemented
* Remote ColdFusion server file system access.
* Remote ColdFusion server database access.
* Remote ColdFusion server debugger.
* Remove ColdFusion server info.

## TODO
* Remote ColdFusion server adminapi settings.
* Remote ColdFusion server graph(chart).
* Remote ColdFusion server webapp security analyzer service.
* Code cleanup.

## CLI test script usage ex:
> `RDS_HOST=rds://localhost:8500 RDS_DSN={some_coldfusion_dsn} RDS_DSN_TABLE={some_table} ./test.sh`

## CLI examples
* List directory - `cfrds ls <rds://{username{:password}@}host{:port}/pathname>`
* Print file content - `cfrds cat <rds://{username{:password}@}host{:port}/pathname>`
* File download - `cfrds download <rds://{username{:password}@}host{:port}/pathname> <destination name/pathname>`
* File upload - `cfrds upload <source_file> <rds://{username{:password}@}host{:port}/pathname>`
* File/dir rename - `cfrds mv <rds://{username{:password}@}host{:port}/pathname> <destination name/pathname>`
* File delete - `cfrds rm <rds://{username{:password}@}host{:port}/pathname>`
* Create directory - `cfrds mkdir <rds://{username{:password}@}host{:port}/pathname>`
* Delete directory - `cfrds rmdir <rds://{username{:password}@}host{:port}/pathname>`
* Get ColdFusion installation directory - `cfrds cfroot <rds://{username{:password}@}host{:port}>`
* Get ColdFusion data source names info - `cfrds dsninfo <rds://{username{:password}@}host{:port}>`
* Get ColdFusion data source name tables info - `cfrds tableinfo <rds://{username{:password}@}host{:port}/dsn>`
* Get ColdFusion data source name table columns info - `cfrds columninfo <rds://{username{:password}@}host{:port}/dsn/table>`
* Get ColdFusion data source name table primary keys info - `cfrds primarykeys <rds://{username{:password}@}host{:port}/dsn/table>`
* Get ColdFusion data source name table foreign keys info - `cfrds primarykeys <rds://{username{:password}@}host{:port}/dsn/table>`
* Get ColdFusion data source name table imported keys info - `cfrds primarykeys <rds://{username{:password}@}host{:port}/dsn/table>`
* Get ColdFusion data source name table exported keys info - `cfrds primarykeys <rds://{username{:password}@}host{:port}/dsn/table>`
* Execute ColdFusion data source name SQL - `cfrds sql <rds://{username{:password}@}host{:port}/dsn> '<SQL>'`
* Get ColdFusion data source name SQL metadata - `cfrds sqlmetadata <rds://{username{:password}@}host{:port}/dsn> '<SQL>'`
* Get ColdFusion data source name database info - `cfrds dbdescription <rds://{username{:password}@}host{:port}/dsn>`
* Get ColdFusion server info - `cfrds ide_default <rds://{username{:password}@}host{:port}> <version 0-15>`

## Build
> git clone https://github.com/bokic/cfrds.git
> 
> cd cfrds
> 
> cmake -B build
> 
> cmake --build build
> 
> bin/cfrds

## Installation
* Linux
  * arch
    > `yay -S cfrds`
  * Ubuntu
    > `sudo add-apt-repository ppa:bbarbulovski-gmail/cfrds`
    > 
    > `sudo apt update`
    >
    >  `sudo apt install cfrds`
* Windows
    > See project [releases](https://github.com/bokic/cfrds/releases) page.
* MacOS
  * homebrew - Work in progress.
