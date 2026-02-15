# cfrds - ColdFusion RDS protocol library with python bindings and CLI interface.

## Project description
cfrds is a library and cli executable designed to communicate to ColdFusion server via Adobe RDS protocol. More info regarding the protocol at https://helpx.adobe.com/coldfusion/coldfusion-builder-extension-for-visual-studio-code/rds-support.html.

Project is written in C. It's portable and can be compiled for Windows, Linux, and MacOS, Intel and ARM 32/64bit targets.

Some of the features RDS protocol supports:
* Server side file access(list dir, upload, download file).
* Run security analyzer on CFML application.
* Enumerating databases connections/tables/table structures(etc)
* Executing SQL on server side.
* Remote ColdFusion debugger.
* Fetch CFML server info.
* Fetch CFML server adminapi settings.
* other...

## Implemented
* Remote ColdFusion server file system access.
* Remote ColdFusion server database access.
* Remote ColdFusion server debugger.
* Remote ColdFusion server info.
* Remote ColdFusion server adminapi settings.
* Remote ColdFusion server webapp security analyzer service.

## TODO
* Remote ColdFusion server graph(chart).
* cfrds cli print all output in json format(--json tail argument).
* Code cleanup.

## CLI test script usage ex:
> `RDS_HOST=<rds://[username[:password]@]host[:port]> RDS_DSN=<dsn_name> RDS_DSN_TABLE=<table_name> ./test.sh`

## CLI examples
* List directory - `cfrds ls <rds://[username[:password]@]host[:port]/[path]>`
* Print file content - `cfrds cat <rds://[username[:password]@]host[:port]</pathname>>`
* File download - `cfrds download <rds://[username[:password]@]host[:port]</pathname>> <local_pathname>`
* File upload - `cfrds upload <local_pathname> <rds://[username[:password]@]host[:port]</pathname>>`
* File/dir rename - `cfrds mv <rds://[username[:password]@]host[:port]</pathname>> <new_pathname>`
* File delete - `cfrds rm <rds://[username[:password]@]host[:port]</pathname>>`
* Create directory - `cfrds mkdir <rds://[username[:password]@]host[:port]</path>>`
* Delete directory - `cfrds rmdir <rds://[username[:password]@]host[:port]</path>>`
* Get ColdFusion installation directory - `cfrds cfroot <rds://[username[:password]@]host[:port]>`
* Get ColdFusion data source names info - `cfrds dsninfo <rds://[username[:password]@]host[:port]>`
* Get ColdFusion data source name tables info - `cfrds tableinfo <rds://[username[:password]@]host[:port]/<dsn_name>>`
* Get ColdFusion data source name table columns info - `cfrds columninfo <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`
* Get ColdFusion data source name table primary keys info - `cfrds primarykeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`
* Get ColdFusion data source name table foreign keys info - `cfrds foreignkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`
* Get ColdFusion data source name table imported keys info - `cfrds importedkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`
* Get ColdFusion data source name table exported keys info - `cfrds exportedkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`
* Execute ColdFusion data source name SQL - `cfrds sql <rds://[username[:password]@]host[:port]/<dsn_name>> "<sql_statement>"`
* Get ColdFusion data source name SQL metadata - `cfrds sqlmetadata <rds://[username[:password]@]host[:port]/<dsn_name>> "<sql_statement>"`
* Get ColdFusion data source supported SQL commands - `cfrds sqlsupportedcommands <rds://[username[:password]@]host[:port]/<dsn_name>>`
* Get ColdFusion data source name database info - `cfrds dbdescription <rds://[username[:password]@]host[:port]/<dsn_name>>`
* Get ColdFusion server info - `cfrds ide_default <rds://[username[:password]@]host[:port]> <version>`
* Get ColdFusion server debugging log property - `cfrds adminapi <rds://[username[:password]@]host[:port]> debugging_getlogproperty <log_directory>`
* Get ColdFusion server extensions custom tag paths - `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_getcustomtagpaths`
* Get ColdFusion server extensions mappings - `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_getmappings`
* Set ColdFusion server extensions mapping - `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_setmapping <mapping_name> <mapping_path>`
* Delete ColdFusion server extensions mapping - `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_deletemapping <mapping_name>`

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
  * homebrew
    > `brew install --build-from-source ./MacOS/cfrds.rb`
