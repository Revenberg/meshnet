-- Remove duplicate groups (keep the first one, delete others)
DELETE g1 FROM `groups` g1
INNER JOIN `groups` g2 
WHERE g1.name = g2.name AND g1.id > g2.id;

-- Remove duplicate users (keep the first one, delete others)
DELETE u1 FROM users u1
INNER JOIN users u2 
WHERE u1.username = u2.username AND u1.id > u2.id;

-- Add UNIQUE constraint on group names (if not already exists)
ALTER TABLE `groups` ADD UNIQUE INDEX unique_group_name (name);

-- Add UNIQUE constraint on usernames (if not already exists)
ALTER TABLE users ADD UNIQUE INDEX unique_username (username);
