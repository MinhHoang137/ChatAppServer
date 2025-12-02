-- SQLite compatible schema

create table if not exists Users (
    UserID INTEGER PRIMARY KEY AUTOINCREMENT,
    Username TEXT not null unique,
    PasswordHash TEXT not null,
    Status INTEGER default 0, -- 0: offline, 1: online
    CreatedAt DATETIME default CURRENT_TIMESTAMP
);

create table if not exists Messages (
    MessageID INTEGER PRIMARY KEY AUTOINCREMENT,
    SenderID INTEGER not null,
    ReceiverID INTEGER not null,
    Content TEXT not null,
    SentAt DATETIME default CURRENT_TIMESTAMP,
    foreign key (SenderID) references Users(UserID),
    foreign key (ReceiverID) references Users(UserID)
);

create table if not exists Friendships (
    UserID1 INTEGER not null,
    UserID2 INTEGER not null,
    RequesterID INTEGER not null, -- ID of the user who sent the friend request
    Status INTEGER not null, -- 0: pending, 1: accepted, 2: blocked
    CreatedAt DATETIME default CURRENT_TIMESTAMP,
    primary key (UserID1, UserID2),
    foreign key (UserID1) references Users(UserID),
    foreign key (UserID2) references Users(UserID)
    -- foreign key (RequesterID) references Users(UserID)
);

create table if not exists Groups (
    GroupID INTEGER PRIMARY KEY AUTOINCREMENT,
    GroupName TEXT not null,
    CreatedAt DATETIME default CURRENT_TIMESTAMP
);

create table if not exists GroupMembers (
    GroupID INTEGER not null,
    UserID INTEGER not null,
    JoinedAt DATETIME default CURRENT_TIMESTAMP,
    primary key (GroupID, UserID),
    foreign key (GroupID) references Groups(GroupID),
    foreign key (UserID) references Users(UserID)
);

create table if not exists GroupMessages (
    GroupMessageID INTEGER PRIMARY KEY AUTOINCREMENT,
    GroupID INTEGER not null,
    SenderID INTEGER not null,
    Content TEXT not null,
    SentAt DATETIME default CURRENT_TIMESTAMP,
    foreign key (GroupID) references Groups(GroupID),
    foreign key (SenderID) references Users(UserID)
);

-- register user
insert into Users (Username, PasswordHash, Status) values ('alice', 'hashed_password_1', 1);

-- login user
SELECT PasswordHash, UserID FROM Users WHERE Username = 'alice' AND PasswordHash = 'hashed_password_1';
