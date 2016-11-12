# libsaxbospiral - Contributing

Thanks for considering contributing to libsaxbospiral!

To get started, [sign the Contributor License Agreement](https://www.clahub.com/agreements/saxbophone/libsaxbospiral).

> [A copy of the CLA](CLA_INDIVIDUAL.md) is also available in this repository.

Here are some tips and general info about contributing to this project. Following these tips will increase the likelihood of getting a speedy PR :smile:

## Checklist

Before you contribute, check the work you're about to do fulfils one of these criteria:

1. Implements a feature represented by an accepted issue in the project's [Github issue tracker](https://github.com/saxbophone/libsaxbospiral/issues)
2. Fixes a bug you discovered (preferably an issue should be created too, but for small or critical bugs, this may not be needed).

Please also check that the work is not already being undertaken by someone else.

## Code Style

I haven't yet formalised the style guide for this project, but this is something I plan to do in the future. Until then, if you could try your best to follow the style of existing code, that will help a great deal.

In addition, please make sure:

- You commit files with Unix Line-endings (`\n` `<LF>` `0x0a`)
- Each text file committed has a trailing newline at the end
- Your C code is compliant to the ISO C99 and C11 standards
- C source code is indented with 4 spaces per indentation level (no tabs)
- Public functions are prototyped in the correct C Header file, all private declarations are declared `static`
- Every public function (and ideally private too) has an accompanying explanatory comment

## Testing

The unit tests for libsaxbospiral currently all reside in one C source file, `tests.c`. This isn't ideal, and I'm planning to clean these up at some point. Build and run the unit tests when you first pull down the code, rebuild and run them again when you've made your changes. Changes adding larger pieces of functionality will likely have additional tests requested for them, or an offer made to write the tests for them.

Pull requests also go through [Travis CI](https://travis-ci.org/) for automated testing.