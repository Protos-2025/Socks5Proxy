# Contributing guidelines

Note, all the following guidelines have an associated githook to aid follow the best practices and conventions described.

Please consider setting your githooks configuration to the one contained within this repository.

```shell
git config core.hooksPath ./.githooks
```

Or modify your global .gitconfig to include the following, which will set your git settings while working on this project.

```
[includeIf "gitdir:path-of-the-local-repository"]
	path = path-of-the-local-repository/.gitconfig
```

## C Style

This repo uses `clang-format` (based on Google's cpp format). All code shall be formatted accordingly.

To ensure uniform settings, clang-format shall only be ran inside the `compile` container, as seen in [precommit](./.githooks/pre-commit)

The specific clang-format configurations can be found in the [.clang-format](./.clang-format) file

## Bypassing hooks

> [!CAUTION]
> Hooks help maintain style, consistency, and functionality in check. Bypassing them should be a last resort and done with care.

In order to bypass the hooks, use the `--no-verify` option.

Example:

- `git commit -a -m "broken" --no-verify` (bypasses conventional commits and Google Java formatter hooks).
- `git push --no-verify` (does not run tests against the code before pushing)
