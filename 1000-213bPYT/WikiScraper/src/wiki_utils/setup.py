from setuptools import find_packages, setup

setup(
    name="wiki_utils",
    version="1.0.0",
    author="Adam Bęcłowicz",
    description="Utility functions and classes for the WikiScraper",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    packages=find_packages(),
    python_requires=">=3.12.4",
    install_requires=[],
)
