# Geode Mod Comments Server

A server for the [Mod Comments](https://github.com/user95401/Geode-Mod-Comments) Geode modification, providing ability to comment the Geometry Dash mods.

## Features

- User authentication via [Argon](https://github.com/globedgd/argon)
- CRUD operations for comments
- Auth tests rate limiting (valid tokens saved for 12 hours)
- JSON files based data storage

## Requirements

- PHP7
- Apache web server
- Internet access for [Argon](https://github.com/globedgd/argon) authentication

## API Documentation

### Authentication
All write operations require token for [Argon](https://github.com/globedgd/argon). Validated tokens are cached locally for 12 hours.

### Endpoints

#### GET [`/`](/)
- **Description**: Returns this documentation
- **Response**: HTML documentation page

#### GET [`/?get={modID}`](/?get={modID})
- **Description**: Retrieves all comments for a specific mod
- **Parameters**:
  - `modID`: The ID of the mod to fetch comments for
- **Response**: JSON array of comment objects
- **Example**:
  ```json
  [
    {
      "id": 1234567890,
      "user": "Player1",
      "body": "Great mod!",
      "accountID": 12345,
      "userID": 67890
    }
  ]
  ```

#### POST [`/?post={modID}`](/?post={modID})
- **Description**: Posts a new comment for a mod
- **Required POST parameters**:
  - `account_id`: GD Account ID (integer)
  - `user_id`: GD User ID (integer)
  - `username`: GD Username (string)
  - `token`: Argon authentication token (string)
  - `body`: Comment content (string, 1-500 characters)
- **Success response**: "Comment posted"
- **Error responses**:
  - "Invalid token: [reason]"
  - "Mismatched username, please refresh login in account settings"
  - "Invalid body"

#### POST [`/?update={modID}`](/?update={modID})
- **Description**: Updates an existing comment
- **Required POST parameters**:
  - `account_id`: GD Account ID (integer)
  - `user_id`: GD User ID (integer)
  - `username`: GD Username (string)
  - `token`: Argon authentication token (string)
  - `comment_id`: ID of comment to update (integer)
  - `body`: New comment content (string, 1-500 characters)
- **Success response**: "Updated"
- **Error responses**:
  - "Not found"
  - "Not your comment"
  - "Invalid body"

#### POST [`/?delete={modID}`](/?delete={modID})
- **Description**: Deletes a comment
- **Required POST parameters**:
  - `account_id`: GD Account ID (integer)
  - `user_id`: GD User ID (integer)
  - `username`: GD Username (string)
  - `token`: Argon authentication token (string)
  - `comment_id`: ID of comment to delete (integer)
- **Success response**: "Deleted"
- **Error responses**:
  - "Not found"
  - "Not your comment"

## Data Structure

Comments are stored in the following structure:
```
data/
    tokens.json             # Cached authentication tokens
    comments/               # Comments threads as mod ID's
        {modID}/            # One directory per mod
            {timestamp}     # Individual comment files named by timestamp
```

## Security

- All user input is sanitized
- Authentication tokens are validated through Argon
- Local token cache expires after 12 hours
- Comment bodies are limited to 500 characters
- Mod IDs are sanitized to prevent directory traversal

## Limitations (TO DO...)

- No pagination for comment retrieval
- No user blocking/reporting system

## Deployment

1. Clone this repository to your web server
2. Ensure the `data/` directory is writable by the web server
3. Configure Apache to serve the `index.php` file
4. Verify Argon API connectivity

## Dependencies

- [Argon Server](https://argon.globed.dev)
