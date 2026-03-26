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
        line = line.replace("db >", "", 1).rstrip()
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
        for i in range(1, 1402)
    ]
    script.append(".exit")

    result = run_script(script)
    assert "Need to implement splitting internal node" in result


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

def test_constants():
    remove_test_db()
    script = """.constants
.exit
"""

    result = run_script(script)

    expected = [
        "Constants:",
        "ROW_SIZE: 296",
        "COMMON_NODE_HEADER_SIZE: 6",
        "LEAF_NODE_HEADER_SIZE: 14",
        "LEAF_NODE_CELL_SIZE: 300",
        "LEAF_NODE_SPACE_FOR_CELLS: 4082",
        "LEAF_NODE_MAX_CELLS: 13",
    ]

    for line in expected:
        assert line in result

def test_prints_one_node_btree():
    remove_test_db()
    script = [
        "insert 3 user3 person3@example.com",
        "insert 1 user1 person1@example.com",
        "insert 2 user2 person2@example.com",
        ".btree",
        ".exit",
    ]

    result = run_script(script)

    expected = [
        "Executed.",
        "Executed.",
        "Executed.",
        "Tree:",
        "- leaf (size 3)",
        "  - 1",
        "  - 2",
        "  - 3",
    ]

    for line in expected:
        assert line in result

def test_duplicate_id():
    remove_test_db()
    script = [
        "insert 1 user1 person1@example.com",
        "insert 1 user1 person1@example.com",
        "select",
        ".exit",
    ]

    result = run_script(script)

    expected = [
        "Executed.",
        "Error: Duplicate key.",
        "(1, user1, person1@example.com)",
        "Executed.",
    ]

    for line in expected:
        assert line in result

def test_prints_three_leaf_btree():
    remove_test_db()
    script = [
        f"insert {i} user{i} person{i}@example.com"
        for i in range(1, 15)
    ]
    script.append(".btree")
    script.append("insert 15 user15 person15@example.com")
    script.append(".exit")

    result = run_script(script)

    result_subset = result[14:]

    expected = [
        "Tree:",
        "- internal (size 1)",
        "  - leaf (size 7)",
        "    - 1",
        "    - 2",
        "    - 3",
        "    - 4",
        "    - 5",
        "    - 6",
        "    - 7",
        "  - key 7",
        "  - leaf (size 7)",
        "    - 8",
        "    - 9",
        "    - 10",
        "    - 11",
        "    - 12",
        "    - 13",
        "    - 14",
        "Executed.",
    ]

    for line in expected:
        assert line in result_subset

def test_select_multi_level_tree():
    remove_test_db()

    script = [
        f"insert {i} user{i} person{i}@example.com"
        for i in range(1, 16)
    ]
    script.append("select")
    script.append(".exit")

    result = run_script(script)

    result_subset = result[15:]

    expected = [
        "(1, user1, person1@example.com)",
        "(2, user2, person2@example.com)",
        "(3, user3, person3@example.com)",
        "(4, user4, person4@example.com)",
        "(5, user5, person5@example.com)",
        "(6, user6, person6@example.com)",
        "(7, user7, person7@example.com)",
        "(8, user8, person8@example.com)",
        "(9, user9, person9@example.com)",
        "(10, user10, person10@example.com)",
        "(11, user11, person11@example.com)",
        "(12, user12, person12@example.com)",
        "(13, user13, person13@example.com)",
        "(14, user14, person14@example.com)",
        "(15, user15, person15@example.com)",
        "Executed.",
    ]

    for line in expected:
        assert line in result_subset

def test_prints_four_leaf_btree():
    remove_test_db()

    script = [
        "insert 18 user18 person18@example.com",
        "insert 7 user7 person7@example.com",
        "insert 10 user10 person10@example.com",
        "insert 29 user29 person29@example.com",
        "insert 23 user23 person23@example.com",
        "insert 4 user4 person4@example.com",
        "insert 14 user14 person14@example.com",
        "insert 30 user30 person30@example.com",
        "insert 15 user15 person15@example.com",
        "insert 26 user26 person26@example.com",
        "insert 22 user22 person22@example.com",
        "insert 19 user19 person19@example.com",
        "insert 2 user2 person2@example.com",
        "insert 1 user1 person1@example.com",
        "insert 21 user21 person21@example.com",
        "insert 11 user11 person11@example.com",
        "insert 6 user6 person6@example.com",
        "insert 20 user20 person20@example.com",
        "insert 5 user5 person5@example.com",
        "insert 8 user8 person8@example.com",
        "insert 9 user9 person9@example.com",
        "insert 3 user3 person3@example.com",
        "insert 12 user12 person12@example.com",
        "insert 27 user27 person27@example.com",
        "insert 17 user17 person17@example.com",
        "insert 16 user16 person16@example.com",
        "insert 13 user13 person13@example.com",
        "insert 24 user24 person24@example.com",
        "insert 25 user25 person25@example.com",
        "insert 28 user28 person28@example.com",
        ".btree",
        ".exit",
    ]

    result = run_script(script)

    expected = [
            "Tree:",
            "- internal (size 3)",
            "  - leaf (size 7)",
            "    - 1",
            "    - 2",
            "    - 3",
            "    - 4",
            "    - 5",
            "    - 6",
            "    - 7",
            "  - key 7",
            "  - leaf (size 8)",
            "    - 8",
            "    - 9",
            "    - 10",
            "    - 11",
            "    - 12",
            "    - 13",
            "    - 14",
            "    - 15",
            "  - key 15",
            "  - leaf (size 7)",
            "    - 16",
            "    - 17",
            "    - 18",
            "    - 19",
            "    - 20",
            "    - 21",
            "    - 22",
            "  - key 22",
            "  - leaf (size 8)",
            "    - 23",
            "    - 24",
            "    - 25",
            "    - 26",
            "    - 27",
            "    - 28",
            "    - 29",
            "    - 30",
        ]

    for line in expected:
        assert line in result

if __name__ == "__main__":
    test_insert_and_select()
    test_keeps_data_after_closing_connection()
    test_table_full()
    test_max_length_strings()
    test_input_too_long()
    test_negative_id()
    test_username_too_long()
    test_email_too_long()
    test_constants()
    test_prints_one_node_btree()
    test_duplicate_id()
    test_prints_three_leaf_btree()
    test_select_multi_level_tree()
    test_prints_four_leaf_btree()
    print("All tests passed")
