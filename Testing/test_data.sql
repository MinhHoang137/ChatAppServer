-- Sample data for ChatAppServer (SQLite)
-- Run Database.sql first to create tables.

PRAGMA foreign_keys = ON;
SAVEPOINT load_test_data;

-- Optional: reset existing data for a clean state
DELETE FROM GroupMessages;
DELETE FROM GroupMembers;
DELETE FROM Groups;
DELETE FROM Messages;
DELETE FROM Friendships;
DELETE FROM Users;

-- Users
INSERT INTO Users (Username, PasswordHash, Status) VALUES
    ('alice',   'alice_pass',   1),
    ('bob',     'bob_pass',     1),
    ('charlie', 'charlie_pass', 0),
    ('diana',   'diana_pass',   1),
    ('eve',     'eve_pass',     0);

-- Friendships
-- Status: 0=pending, 1=accepted, 2=blocked
-- Insert friendships without RequesterID for compatibility with schemas lacking the column
INSERT INTO Friendships (UserID1, UserID2, Status, CreatedAt) VALUES
    ((SELECT UserID FROM Users WHERE Username='alice'),   (SELECT UserID FROM Users WHERE Username='bob'),     1, CURRENT_TIMESTAMP),
    ((SELECT UserID FROM Users WHERE Username='alice'),   (SELECT UserID FROM Users WHERE Username='charlie'), 0, CURRENT_TIMESTAMP),
    ((SELECT UserID FROM Users WHERE Username='bob'),     (SELECT UserID FROM Users WHERE Username='diana'),   1, CURRENT_TIMESTAMP),
    ((SELECT UserID FROM Users WHERE Username='charlie'), (SELECT UserID FROM Users WHERE Username='eve'),     0, CURRENT_TIMESTAMP);

-- Direct Messages (between friends/pending for sample)
INSERT INTO Messages (SenderID, ReceiverID, Content, SentAt) VALUES
    ((SELECT UserID FROM Users WHERE Username='alice'), (SELECT UserID FROM Users WHERE Username='bob'),   'Hi Bob! Welcome to ChatApp.',              CURRENT_TIMESTAMP),
    ((SELECT UserID FROM Users WHERE Username='bob'),   (SELECT UserID FROM Users WHERE Username='alice'), 'Hey Alice! Good to be here!',               CURRENT_TIMESTAMP),
    ((SELECT UserID FROM Users WHERE Username='alice'), (SELECT UserID FROM Users WHERE Username='charlie'), 'Hi Charlie, did you get my request?',      CURRENT_TIMESTAMP),
    ((SELECT UserID FROM Users WHERE Username='diana'), (SELECT UserID FROM Users WHERE Username='bob'),   'Hi Bob, meeting at 3 PM?',                  CURRENT_TIMESTAMP),
    ((SELECT UserID FROM Users WHERE Username='bob'),   (SELECT UserID FROM Users WHERE Username='diana'), 'Sure, see you then.',                       CURRENT_TIMESTAMP);

-- Groups
INSERT INTO Groups (GroupName) VALUES
    ('General'),
    ('Dev Team'),
    ('Gaming');

-- Group Members
INSERT INTO GroupMembers (GroupID, UserID, JoinedAt) VALUES
    ((SELECT GroupID FROM Groups WHERE GroupName='General'), (SELECT UserID FROM Users WHERE Username='alice'),   CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='General'), (SELECT UserID FROM Users WHERE Username='bob'),     CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='General'), (SELECT UserID FROM Users WHERE Username='charlie'), CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='General'), (SELECT UserID FROM Users WHERE Username='diana'),   CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='General'), (SELECT UserID FROM Users WHERE Username='eve'),     CURRENT_TIMESTAMP),

    ((SELECT GroupID FROM Groups WHERE GroupName='Dev Team'), (SELECT UserID FROM Users WHERE Username='alice'),  CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='Dev Team'), (SELECT UserID FROM Users WHERE Username='bob'),    CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='Dev Team'), (SELECT UserID FROM Users WHERE Username='diana'),  CURRENT_TIMESTAMP),

    ((SELECT GroupID FROM Groups WHERE GroupName='Gaming'), (SELECT UserID FROM Users WHERE Username='charlie'),  CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='Gaming'), (SELECT UserID FROM Users WHERE Username='eve'),      CURRENT_TIMESTAMP);

-- Group Messages
INSERT INTO GroupMessages (GroupID, SenderID, Content, SentAt) VALUES
    ((SELECT GroupID FROM Groups WHERE GroupName='General'), (SELECT UserID FROM Users WHERE Username='alice'),   'Welcome everyone to General!', CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='General'), (SELECT UserID FROM Users WHERE Username='bob'),     'Hi all 👋',                   CURRENT_TIMESTAMP),

    ((SELECT GroupID FROM Groups WHERE GroupName='Dev Team'), (SELECT UserID FROM Users WHERE Username='alice'), 'Please review PR #42',        CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='Dev Team'), (SELECT UserID FROM Users WHERE Username='diana'), 'Deployment at 5 PM',          CURRENT_TIMESTAMP),

    ((SELECT GroupID FROM Groups WHERE GroupName='Gaming'), (SELECT UserID FROM Users WHERE Username='charlie'), 'Tonight: 8 PM co-op?',        CURRENT_TIMESTAMP),
    ((SELECT GroupID FROM Groups WHERE GroupName='Gaming'), (SELECT UserID FROM Users WHERE Username='eve'),     'Count me in!',                CURRENT_TIMESTAMP);

RELEASE SAVEPOINT load_test_data;
