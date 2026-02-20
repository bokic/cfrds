from distutils.core import Extension, setup


def main():
    setup(
        name="cfrds",
        version="1.0.0",
        description="Python interface for ColdFusion RDS service.",
        author="Boris Barbulovski",
        author_email="bbarbulovski@gmail.com",
        ext_modules=[
            Extension(
                "cfrds",
                [
                    "cfrds.py.c",
                    "../src/cfrds.c",
                    "../src/wddx.c",
                    "../src/cfrds_buffer.c",
                    "../src/cfrds_http.c",
                ],
                include_dirs=["../include", "/usr/include/libxml2", "/usr/include/json-c"],
                libraries=["xml2", "json-c"],
            )
        ],
    )


if __name__ == "__main__":
    main()
