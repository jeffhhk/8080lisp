# Global information
The current product goal and requirements are held in docs/IMPLEMENTATION_PLAN.md

A user request may involve more than one Change.

## Change constraints
These apply to every change.
- Every change has at least one commit.
- One purpose per change
- Repository must remain buildable
- Take reasonable precautions to avoid duplicating code which could be reused.  Do not repeat yourself.  (DRY)
- Every change must specify a set of automated tests (preferably existing ones) or manual verification that can verify the change.
- Never consider git merge or git cherry-pick unless explicitly instructed.

Every commit is an implementation, a refactoring, or a bugfix, but not more than one of these.

### Implementation Change
- Adds new features
- Major code paths validated with tests
- Compared with Bugfix and Refactoring changes, less emphasis on correctness
- Compared with Bugfix and Refactoring changes, less emphasis on minimality
- Bugs may be inadvertently introduced due to new interactions, so despite our best intentions we expect that the bug count will be slightly larger after each feature.
- Minimum changed lines to express feature

### Bugfix Change
- Emphasis on confidence in decreasing the overall bug count/impact
- Emphasis on correctness
- Minimum lines to express correctness

### Refactoring Change
- Emphasis on logical cohesion of the code
- Bug and feature phenomena should be identical before and after the change
- Less emphasis on minimum line count.
- Refactoring may include “move/port” changes: adding code in the new location and deleting the old location in the same change.
- Atomic move rule: If a refactor ports/moves code (whole or partial file), the commit must include both the new location and the deletion of the old location (no follow-up “delete” commit for the same move).
- Deleting superseded code/assets is expected and does not require extra confirmation

## Change constraint remediation
If a task would exceed these limits for a single change:
1. Split the task into smaller backlog items.
2. Update docs/IMPLEMENTATION_BACKLOG.yaml.
3. Implement as many of the resulting backlog items as feasible in the same turn, in priority order.
4. Create a separate compliant commit for each change.
5. Stop only when:
    - the original request is fully completed, or
    - an external blocker prevents further progress.

## Multi-change execution
- “One purpose per change” applies per commit, not per user request.
- When the user asks to implement all remaining backlog, codex should continue through the backlog
item by item, making multiple commits as needed, until the remaining backlog is complete or blocked.
- Do not stop after the first newly split task unless the user asked for partial progress.

## Official Build Commands

See docs/IMPLEMENTATION_PLAN.md

## Task workflow

Throughout your task workflow, never attempt to run an rm command that would require approval unless you have tried (./gradlew clean).  If clean fails, then talk to me about how to improve clean, not how to run your adhoc commands.

Never ask for permission to `git rm` or `git rm -rf`.

2a. Check the repository for code which might be reused in your task.
2b. Implement the task
3a. Build with the Official Build Command
3b. Validate using tests or manual steps
3c.
    For refactors, it is mandatory to remove superseded files/dirs as part of the same change.
    As a double check for refactoring, examine:
        git diff --cached --stat
    Make sure that there are roughly the same number of inserted as deleted lines.  If total lines or specific file lines are significantly different, group which lines added correspond to which lines deleted, and consider the possibility that you have forgotten to remove files/lines from git.

3d. Verify that ephemeral directories are not adding files to git, such as:
    - voirceapp/platforms/android
3e. Please use (./gradlew clean) instead of asking:
        Would you like to run the following command?
        $ rm -rf build android-test/build
    If other clean targets are commonly required, offer to make those instead of asking for oneoff rm permissions.
4. Update docs/DEVELOPMENT_LOG.yaml and docs/DEVELOPMENT_BACKLOG.yaml.  Try to not duplicate information between them.
4b. Update docs/DEVELOPMENT_LOG.yaml
    - Do not include the dates of the changes, since they are already in the logs
    - Use the following format:
            - meta: {_: <kind>, num:<num> guid: <guid>}
            title: <title>
            prompt: |
                <prompt>
            verifications:
                <verifications>
        where:
            <kind> is one of: feature/bug/refactor
            <num> (optional) a small integer.  Omit for new changes.
            <guid> four random characters of the class [a-z0-9]
            <title> is a one line title.  Phrase bugs with the word "should"
            <prompt> is the original prompt used, if available
            <verifications> is a list of verifications performed
    - Include verbatim the prompt used to create the change, including any important prompt completions, but exclude any long code blocks.
    - It's ok for DEVELOPMENT_LOG entries to refer to other entries by guid, but the guid of each new entry must be unique.  Any existing duplicate is an error.
    - For each of the <verifications>, use the following format:
            - verification: <verification>
              result: <brief_result>
        where:
            <verification> is the action taken
            <brief_result> is the test count, if known, e.g. "M tests N failures K skipped"
    - Add new changes to the end of the log.
4c. Update Backlog Items in docs/IMPLEMENTATION_BACKLOG.yaml
    - Reserve docs/IMPLEMENTATION_BACKLOG.yaml for deferred bug fixes and deferred feature requests only.
    - For any newly discovered confirmed bug, add or update a bug Backlog Item before finishing the change.
    - For any newly discovered suspected bug with reproducible evidence or concrete technical indicators, add or update a bug Backlog Item before finishing the change.
    - Do not create a bug Backlog Item for a test-authoring mistake or an issue that was disproved during investigation.
    - Prefer updating an existing Backlog Item over creating a duplicate when the issue appears to be the same underlying problem.
    - Bug Backlog Items must be explicitly marked as bugs and should summarize:
        - the observed behavior
        - the evidence
        - the current confidence or uncertainty
        - any known workaround or containment
    - If an existing Backlog Item gains materially new evidence, update that item instead of adding a new one.
    - Use the following format:
            - meta: {_: <kind>, id: <task id>, status: <status>, priority: <priority>, guid: <guid>}
              title: <title>
              expected_behavior: |
                  <expected behavior>
              evidence:
                  <evidence items>
              acceptance:                 # optional field
                  <acceptance items>
              related:                    # optional field
                  <related items>
              notes: |                    # optional field
                  <notes>
        where:
            <kind> is one of: bug/feature/refactor/test/docs/investigation
            <task id> is a stable task identifier such as PORT-0002E
            <status> is one of: open/in_progress/blocked/done/wont_fix
            <priority> is one of: high/medium/low
            <guid> four random characters of the class [a-z0-9]
            <title> is a one line title.  Phrase bugs with the word "should"
            <expected behavior> is a concise statement of the expected behavior
            <evidence items> is a list of evidence entries for or against the expected behavior
            <acceptance items> is a list of conditions that would make the item complete
            <related items> is an optional list of related task IDs, development log guids, or file paths
            <notes> is optional extra context such as uncertainty, workaround, or containment
    - For each of the <evidence items>, use a brief the concrete observation supporting the item
    - For each of the <acceptance items>, use a brief concrete statement.
    - For each of the <related items>, use one of the following formats:
            - task: <task id>
            - development_log: <guid>
            - file: <path>
    - Add new Backlog Items to the end of the file.
    - Keep task IDs unique.  Updating an existing item is preferred to creating a duplicate for the same underlying issue.
5. If you have changed build/run commands, udate docs/BUILD.md
6. Commit using the task ID and one of the following three forms:
    - implementation: <task ID>: <description>
    - fix: <task ID>: <description>
    - refactor: <task ID>: <description>
7. If in the course of making change you noticed any opportunities to delete dead code, list those at the end of your report.

## Testing strategy

The normative testing policy lives in the heading "Testing strategy" in IMPLEMENTATION_PLAN.md.

AGENTS.md should only add agent-execution requirements that are specific to codex behavior and do not belong in the product/testing policy itself.

### Test Double Check Procedure
It is part of testing to assess the impact of a change on each automated test, its target code, and its environment.

For each test significantly impacted by a change, undergo the following Test Double Check Procedure.  When following the Test Double Check Procedure, codex MUST execute it end-to-end without asking for confirmation and without disclaimers.
    1) Edit the test to artifically throw an exception just before it declares final success.
    2) Make sure the test fails with the artificial exception.
    3) Remove the artificial exception throw by an appropriate means.  The artificial exception throw must not be present in final commit.  It may be removed as an uncommitted working-tree change, or as a separate intermediate commit that is squashed/rebased away before final delivery.
    4) Make sure the test passes

While the user may specially request Test Double Check Procedure, you are fully responsible for autonomously undertaking the procedure at appropriate junctures.
