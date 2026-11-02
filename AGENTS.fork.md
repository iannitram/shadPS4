# Rules for agents in this repo (our pipeline)

Everything you need to work here is in this repo. The full pipeline and rules live in the pipeline repo (private): `PIPELINE.md`, `RULES.md`, `RUBRIC.md`, `roles/`; this is the short copy. Upstream's own `AGENTS.md` still applies to upstream's code.

This fork is the shared PS4 graphics core my PS4 game ports build from, plus the package extractor (`SPEC.md`). Upstream's work comes in through the daily upstream merge; we build only our delta.

1. **Find the stage** in `PIPELINE.md`. The spec is `SPEC.md`, the plan `ROADMAP.md`, the code map `docs/code-map.md`, our history `CHANGELOG.fork.md`.
2. **Work from an issue.** Take one that is not `in-progress` and not blocked; add `in-progress` before you start. Its body is your brief; an Easy issue has a "Qwen brief" section and its locked tests (commit them unchanged before the code).
3. **Your own platform's issues first.** Mac, Windows and Linux all matter: say which you built on.
4. **Prove it**: build it, run the issue's tests, then open the PR with that output in its body. Build and tests: `docs/code-map.md`, "Build and test".
5. **Label the PR** `by:<machine>` and `model:<model>`, and write `Closes #N`.
6. **A failed job** gets a comment on the issue saying what happened, and the issue moves up one size (Easy -> Medium -> Hard).
7. **Keep upstream's files close to upstream.** Put our code in our own files (`gpucore/`, `tools/pkg-extract/`, `src/core/file_format/pkg*`); when an upstream file must change, keep the edit small, mark it `// fork:` with a reason, and list the file in `docs/fork-delta.md`. Never rewrite upstream's README, docs or other project files.
8. **No per-game code in the core.** A game changes core behaviour only through the hook table (`gpucore/include/shadps4_gpucore_hooks.h`).
9. **Never post upstream.** Upstream asks for no AI-written contributions: push only to this fork (`origin`); no PRs, issues, comments or reports on shadps4-emu.
10. **No game files, keys or firmware**, ever: not in code, tests, logs, issues or releases. Tests build their own packages from keys they generate.
11. **No private details** in code, commits or PR text: no names, e-mail addresses, home paths or machine names. Commit as the no-reply identity.
