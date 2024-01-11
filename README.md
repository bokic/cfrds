# cfrds - ColdFusion RDS protocol library and CLI
cfrds is a library and cli executable designed to communicate to ColdFusion server via Adobe RDS protocol. More info regarding the protocol at https://helpx.adobe.com/coldfusion/coldfusion-builder-extension-for-visual-studio-code/rds-support.html.
Project is written in C. It's portable and can be compiled for Windows, Linux, and MacOS, Intel and ARM 32/64bit targets .

Some of the features RDS protocol supports:
* Server side file access(list dir, upload, download file).
* Enumerating databases connections/tables/table structures(etc)
* Executing SQL on server side.
* Remote ColdFusion debugger.
* other...


## Some CLI examples
* List directory - `cfrds ls <rds://{username}:{password}@{host}:{port}/{pathname}>`
* File download - `cfrds download <rds://{username}:{password}@{host}:{port}/{pathname}> <destination_file>`
* File upload - `cfrds upload <source_file> <rds://{username}:{password}@{host}:{port}/{pathname}>`

## Build
> cd \<root of the project\>
>
> mkdir build
> 
> cmake -Bbuild -G Ninja
> 
> ninja -Cbuild
