import subprocess

def run_script(commands):
    process = subprocess.Popen(
        ["./db"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    output, error = process.communicate(commands)

    lines = output.splitlines()

    cleaned = []
    for line in lines:
        line = line.replace("db >", "").strip()
        if line:
            cleaned.append(line)

    return cleaned

def test_insert_and_select():
    script = """insert 1 user1 person1@example.com
select
.exit
"""

    result = run_script(script)

    expected = [
        "Executed.",
        "(1, user1, person1@example.com)",
        "Executed."
    ]

    for line in expected:
        assert line in result

def test_table_full():
    script = "\n".join(
        f"insert {i} user{i} person{i}@example.com"
        for i in range(1, 1302)
    )
    script += "\n.exit\n"

    result = run_script(script)

    assert result.count("Executed.") == 1300
    assert result[-1] == "Error: Table full."

def test_max_length_strings():
    long_username = "a" * 32
    long_email = "a" * 255

    script = f"""insert 1 {long_username} {long_email}
select
.exit
"""

    result = run_script(script)

    expected_row = f"(1, {long_username}, {long_email})"
    assert "Executed." in result
    assert expected_row in result

if __name__ == "__main__":
    test_insert_and_select()
    test_table_full()
    test_max_length_strings()
    print("All tests passed") 
