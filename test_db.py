import os
import subprocess


TEST_DB = "test.db"


def remove_test_db():
    if os.path.exists(TEST_DB):
        os.remove(TEST_DB)


def run_script(commands, db_name=TEST_DB):
    process = subprocess.Popen(
        ["./db", db_name],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    if isinstance(commands, list):
        commands = "\n".join(commands) + "\n"

    output, error = process.communicate(commands)

    lines = output.splitlines()

    cleaned = []
    for line in lines:
        line = line.replace("db >", "").strip()
        if line:
            cleaned.append(line)

    return cleaned


def test_insert_and_select():
    remove_test_db()

    result = run_script([
        "insert 1 user1 person1@example.com",
        "select",
        ".exit",
    ])

    expected = [
        "Executed.",
        "(1, user1, person1@example.com)",
        "Executed."
    ]

    for line in expected:
        assert line in result


def test_keeps_data_after_closing_connection():
    remove_test_db()

    result1 = run_script([
        "insert 1 user1 person1@example.com",
        ".exit",
    ])

    assert "Executed." in result1

    result2 = run_script([
        "select",
        ".exit",
    ])

    assert "(1, user1, person1@example.com)" in result2
    assert "Executed." in result2


def test_table_full():
    remove_test_db()

    script = [
        f"insert {i} user{i} person{i}@example.com"
        for i in range(1, 1302)
    ]
    script.append(".exit")

    result = run_script(script)

    assert result.count("Executed.") == 1300
    assert result[-1] == "Error: Table full."


def test_max_length_strings():
    remove_test_db()

    long_username = "a" * 32
    long_email = "a" * 255

    result = run_script([
        f"insert 1 {long_username} {long_email}",
        "select",
        ".exit",
    ])

    expected_row = f"(1, {long_username}, {long_email})"
    assert "Executed." in result
    assert expected_row in result


def test_input_too_long():
    remove_test_db()

    long_input = "a" * 400

    result = run_script([
        long_input,
        ".exit",
    ])

    assert "Input too long." in result


def test_negative_id():
    remove_test_db()

    result = run_script([
        "insert -1 user person@example.com",
        ".exit",
    ])

    assert "ID must be positive." in result


def test_username_too_long():
    remove_test_db()

    long_username = "a" * 33

    result = run_script([
        f"insert 1 {long_username} person@example.com",
        ".exit",
    ])

    assert "String is too long." in result


def test_email_too_long():
    remove_test_db()

    long_email = "a" * 256

    result = run_script([
        f"insert 1 user {long_email}",
        ".exit",
    ])

    assert "String is too long." in result


if __name__ == "__main__":
    test_insert_and_select()
    test_keeps_data_after_closing_connection()
    test_table_full()
    test_max_length_strings()
    test_input_too_long()
    test_negative_id()
    test_username_too_long()
    test_email_too_long()
    print("All tests passed")