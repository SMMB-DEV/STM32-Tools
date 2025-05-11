# Simplified Semantic Versioning 0.2.0

This versioning scheme is mostly identical to [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) with limited major, minor, and patch versions (each max. 255) and fewer and more strict [pre-release identifiers](https://semver.org/spec/v2.0.0.html#spec-item-9). [Build metadata](https://semver.org/spec/v2.0.0.html#spec-item-10) cannot be displayed using this format.

If a patch version reaches 255, the next version will increment the minor version and reset the patch to 0 even if it's not actually a minor release. Similarly, after a minor version of 255, the major version is incremented and the minor and patch versions are reset to 0. If the major version reaches 255 you are probably doing something wrong because no one needs 256 major versions.

## Storage

The version can be stored (and transferred) as a 32-bit number divided into four 8-bit sections. Each section holds part of the version information.

|	8 (MSB)	|	8		|	8		|	8 (LSB)					|
|-----------|-----------|-----------|---------------------------|
|	Major	|	Minor	|	Patch	|	Pre-release identifier	|

This makes comparing two versions as easy as comparing two 32-bit numbers while taking into account their precedence.

### Pre-release identifiers

Here are the **only** valid pre-release identifiers and their associated number:

|	Number	|	Text Form					|	Meaning								|
|-----------|-------------------------------|---------------------------------------|
|	255		|								|	Normal version (not pre-release)	|
|	97-127	|	-rc*x*						|	Release candidate - x from 1 to 31	|
|	96		|	-rc, -rc0, -gamma, -delta	|	Release candidate					|
|	65-95	|	-beta*x*					|	Beta version - x from 1 to 31		|
|	64		|	-beta, -beta0				|	Beta version						|
|	33-63	|	-alpha*x*					|	Alpha version - x from 1 to 31		|
|	32		|	-alpha, -alpha0				|	Alpha version						|
|	0		|	-x							|	Unspecified (not a valid pre-release but when comparing versions, it means that pre-release is not considered)	|

The pre-release usually goes from -alpha to -beta and the to -rc and finally to a normal version but **it can *skip* any of these states**.
