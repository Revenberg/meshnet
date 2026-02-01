const bcrypt = require('bcryptjs');
const mysql = require('mysql2/promise');

async function updatePassword() {
  try {
    const connection = await mysql.createConnection({
      host: 'mysql',
      user: 'meshnet',
      password: 'meshnet_secure_pwd',
      database: 'meshnet'
    });

    // Generate hash for admin123
    const hash = await bcrypt.hash('admin123', 10);
    console.log('Generated hash:', hash);

    // Update admin user
    await connection.execute(
      'UPDATE users SET passwordHash = ? WHERE username = ?',
      [hash, 'admin']
    );

    console.log('Admin password updated successfully');
    await connection.end();
  } catch (error) {
    console.error('Error:', error.message);
  }
}

updatePassword();
