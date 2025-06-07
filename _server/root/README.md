# Geode Mod Comments Server

The server for Mod Comments geode modification.

Built for PHP7 Apache server.

Uses [Argon](https://argon.globed.dev) for authentication.

## API

### GET /?get={id}

Returns comments for the specified mod ID.

### POST /?post={id}

Posts a new comment for the specified mod ID.
- `account_id`: The ID of the GD Account.
- `user_id`: The user ID of the GD Account.
- `username`: The username of the GD Account.
- `token`: The Argon authentication token.
- `body`: The body of the comment.

### POST /?update={id}

Updates an existing comment for the specified mod ID.
- `account_id`: The ID of the GD Account.
- `user_id`: The user ID of the GD Account.
- `username`: The username of the GD Account.
- `token`: The Argon authentication token.
- `comment_id`: The ID of the comment to update.
- `body`: The new body of the comment.

### POST /?delete={id}

Deletes a comment for the specified mod ID.
- `account_id`: The ID of the GD Account.
- `user_id`: The user ID of the GD Account.
- `username`: The username of the GD Account.
- `token`: The Argon authentication token.
- `comment_id`: The ID of the comment to delete.

### POST /?delete={id}

Deletes a comment for the specified mod ID.
- `account_id`: The ID of the GD Account.
- `user_id`: The user ID of the GD Account.
- `username`: The username of the GD Account.
- `token`: The Argon authentication token.
- `comment_id`: The ID of the comment to delete.

