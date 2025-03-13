package com.example.esp32controller

import android.content.SharedPreferences
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.animateContentSize
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.navigation.NavController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.preference.PreferenceManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this)
        setContent {
            ESP32ControllerApp(sharedPreferences)
        }
    }
}

@Composable
fun ESP32ControllerApp(sharedPreferences: SharedPreferences) {
    val navController = rememberNavController()
    val currentUser by remember { mutableStateOf(sharedPreferences.getString("current_user", null)) }

    MaterialTheme {
        NavHost(
            navController = navController,
            startDestination = if (currentUser == null) Screen.Login.route else Screen.Main.route
        ) {
            composable(Screen.Login.route) { LoginScreen(navController, sharedPreferences) }
            composable(Screen.Register.route) { RegisterScreen(navController, sharedPreferences) }
            composable(Screen.Main.route) { MainScreen(navController, sharedPreferences) }
            composable(Screen.LivingRoom.route) { LivingRoomScreen(navController, sharedPreferences) }
            composable(Screen.Garage.route) { GarageScreen(navController, sharedPreferences) }
            composable(Screen.ChangePassword.route) { ChangePasswordScreen(navController, sharedPreferences) }
        }
    }
}

@Composable
fun AnimatedButton(
    text: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    var pressed by remember { mutableStateOf(false) }
    val scale by animateFloatAsState(if (pressed) 0.9f else 1f)

    Button(
        onClick = {
            pressed = true
            onClick()
            pressed = false
        },
        modifier = modifier
            .scale(scale)
            .animateContentSize(),
        colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.primary),
        shape = RoundedCornerShape(8.dp)
    ) {
        Text(text, color = MaterialTheme.colorScheme.onPrimary, fontSize = 16.sp)
    }
}

@Composable
fun LoginScreen(navController: NavController, sharedPreferences: SharedPreferences) {
    var username by remember { mutableStateOf(TextFieldValue("")) }
    var password by remember { mutableStateOf(TextFieldValue("")) }
    var errorMessage by remember { mutableStateOf("") }

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(text = "Login", style = MaterialTheme.typography.headlineMedium)
        Spacer(modifier = Modifier.height(16.dp))
        OutlinedTextField(
            value = username,
            onValueChange = { username = it },
            label = { Text("Username") },
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(8.dp))
        OutlinedTextField(
            value = password,
            onValueChange = { password = it },
            label = { Text("Password") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(16.dp))
        if (errorMessage.isNotEmpty()) {
            Text(text = errorMessage, color = MaterialTheme.colorScheme.error)
            Spacer(modifier = Modifier.height(8.dp))
        }
        Button(
            onClick = {
                val storedPassword = sharedPreferences.getString("password_${username.text}", null)
                if (storedPassword != null && storedPassword == password.text) {
                    sharedPreferences.edit().putString("current_user", username.text).apply()
                    navController.navigate(Screen.Main.route) {
                        popUpTo(Screen.Login.route) { inclusive = true }
                    }
                } else {
                    errorMessage = "Invalid username or password"
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Login")
        }
        Spacer(modifier = Modifier.height(8.dp))
        TextButton(onClick = { navController.navigate(Screen.Register.route) }) {
            Text("Don't have an account? Register")
        }
    }
}

@Composable
fun RegisterScreen(navController: NavController, sharedPreferences: SharedPreferences) {
    var username by remember { mutableStateOf(TextFieldValue("")) }
    var password by remember { mutableStateOf(TextFieldValue("")) }
    var confirmPassword by remember { mutableStateOf(TextFieldValue("")) }
    var errorMessage by remember { mutableStateOf("") }
    var showSuccessDialog by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(text = "Register", style = MaterialTheme.typography.headlineMedium)
        Spacer(modifier = Modifier.height(16.dp))
        OutlinedTextField(
            value = username,
            onValueChange = { username = it },
            label = { Text("Username") },
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(8.dp))
        OutlinedTextField(
            value = password,
            onValueChange = { password = it },
            label = { Text("Password") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(8.dp))
        OutlinedTextField(
            value = confirmPassword,
            onValueChange = { confirmPassword = it },
            label = { Text("Confirm Password") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(16.dp))
        if (errorMessage.isNotEmpty()) {
            Text(text = errorMessage, color = MaterialTheme.colorScheme.error)
            Spacer(modifier = Modifier.height(8.dp))
        }
        Button(
            onClick = {
                when {
                    username.text.isEmpty() || password.text.isEmpty() -> errorMessage = "Please fill all fields"
                    password.text != confirmPassword.text -> errorMessage = "Passwords do not match"
                    sharedPreferences.contains("password_${username.text}") -> errorMessage = "Username already exists"
                    else -> {
                        sharedPreferences.edit().putString("password_${username.text}", password.text).apply()
                        showSuccessDialog = true
                    }
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Register")
        }
        Spacer(modifier = Modifier.height(8.dp))
        TextButton(onClick = { navController.navigate(Screen.Login.route) }) {
            Text("Already have an account? Login")
        }
    }

    if (showSuccessDialog) {
        AlertDialog(
            onDismissRequest = { },
            title = { Text("Success") },
            text = { Text("Tạo tài khoản thành công") },
            confirmButton = {
                TextButton(onClick = {
                    showSuccessDialog = false
                    navController.navigate(Screen.Login.route) {
                        popUpTo(Screen.Register.route) { inclusive = true }
                    }
                }) { Text("OK") }
            }
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(navController: NavController, sharedPreferences: SharedPreferences) {
    val currentUser by remember { mutableStateOf(sharedPreferences.getString("current_user", "Unknown") ?: "Unknown") }
    var menuExpanded by remember { mutableStateOf(false) }
    var showDeleteDialog by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("ESP32 Controller") },
                actions = {
                    IconButton(onClick = { menuExpanded = true }) {
                        Icon(Icons.Default.Settings, contentDescription = "Settings")
                    }
                    DropdownMenu(expanded = menuExpanded, onDismissRequest = { menuExpanded = false }) {
                        DropdownMenuItem(
                            text = { Text("Change Password") },
                            onClick = {
                                menuExpanded = false
                                navController.navigate(Screen.ChangePassword.route)
                            }
                        )
                        DropdownMenuItem(
                            text = { Text("Logout") },
                            onClick = {
                                menuExpanded = false
                                sharedPreferences.edit().remove("current_user").apply()
                                navController.navigate(Screen.Login.route) {
                                    popUpTo(Screen.Main.route) { inclusive = true }
                                }
                            }
                        )
                        DropdownMenuItem(
                            text = { Text("Delete Account") },
                            onClick = {
                                menuExpanded = false
                                showDeleteDialog = true
                            }
                        )
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier.padding(paddingValues).padding(16.dp),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(text = "Logged in as: $currentUser", style = MaterialTheme.typography.bodyLarge)
            Spacer(modifier = Modifier.height(32.dp))
            Button(
                onClick = { navController.navigate(Screen.LivingRoom.route) },
                modifier = Modifier.fillMaxWidth().height(60.dp)
            ) {
                Text("Phòng khách")
            }
            Spacer(modifier = Modifier.height(16.dp))
            Button(
                onClick = { navController.navigate(Screen.Garage.route) },
                modifier = Modifier.fillMaxWidth().height(60.dp)
            ) {
                Text("Gara")
            }
        }

        if (showDeleteDialog) {
            AlertDialog(
                onDismissRequest = { showDeleteDialog = false },
                title = { Text("Confirm Delete") },
                text = { Text("Bạn có chắc chắn muốn xóa tài khoản không?") },
                confirmButton = {
                    TextButton(onClick = {
                        sharedPreferences.edit()
                            .remove("password_$currentUser")
                            .remove("current_user")
                            .apply()
                        showDeleteDialog = false
                        navController.navigate(Screen.Login.route) {
                            popUpTo(Screen.Main.route) { inclusive = true }
                        }
                    }) { Text("Có") }
                },
                dismissButton = { TextButton(onClick = { showDeleteDialog = false }) { Text("Không") } }
            )
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LivingRoomScreen(navController: NavController, sharedPreferences: SharedPreferences) {
    var ipAddress by remember { mutableStateOf(TextFieldValue(sharedPreferences.getString("esp32_ip", "") ?: "")) }
    var status by remember { mutableStateOf("Status: Waiting...") }
    val scope = rememberCoroutineScope()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Phòng khách") },
                navigationIcon = {
                    IconButton(onClick = { navController.navigateUp() }) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .padding(paddingValues)
                .padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Card(
                modifier = Modifier.fillMaxWidth(),
                elevation = CardDefaults.cardElevation(defaultElevation = 4.dp),
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
            ) {
                Column(
                    modifier = Modifier.padding(16.dp),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    OutlinedTextField(
                        value = ipAddress,
                        onValueChange = {
                            ipAddress = it
                            sharedPreferences.edit().putString("esp32_ip", it.text).apply()
                        },
                        label = { Text("ESP32 IP Address") },
                        placeholder = { Text("e.g., 192.168.4.1") },
                        modifier = Modifier.fillMaxWidth(),
                        colors = OutlinedTextFieldDefaults.colors(
                            focusedBorderColor = MaterialTheme.colorScheme.primary,
                            unfocusedBorderColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
                        )
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        AnimatedButton(
                            text = "Bật đèn",
                            onClick = { sendCommand(ipAddress.text, "LIGHT_ON", scope) { status = it } }
                        )
                        AnimatedButton(
                            text = "Tắt đèn",
                            onClick = { sendCommand(ipAddress.text, "LIGHT_OFF", scope) { status = it } }
                        )
                    }
                    Spacer(modifier = Modifier.height(16.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        AnimatedButton(
                            text = "Bật quạt",
                            onClick = { sendCommand(ipAddress.text, "FAN_ON", scope) { status = it } }
                        )
                        AnimatedButton(
                            text = "Tắt quạt",
                            onClick = { sendCommand(ipAddress.text, "FAN_OFF", scope) { status = it } }
                        )
                    }
                }
            }
            Spacer(modifier = Modifier.height(16.dp))
            Text(text = status, style = MaterialTheme.typography.bodyMedium)
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GarageScreen(navController: NavController, sharedPreferences: SharedPreferences) {
    var ipAddress by remember { mutableStateOf(TextFieldValue(sharedPreferences.getString("esp32_ip", "") ?: "")) }
    var status by remember { mutableStateOf("Status: Waiting...") }
    val scope = rememberCoroutineScope()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Gara") },
                navigationIcon = {
                    IconButton(onClick = { navController.navigateUp() }) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .padding(paddingValues)
                .padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Card(
                modifier = Modifier.fillMaxWidth(),
                elevation = CardDefaults.cardElevation(defaultElevation = 4.dp),
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
            ) {
                Column(
                    modifier = Modifier.padding(16.dp),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    OutlinedTextField(
                        value = ipAddress,
                        onValueChange = {
                            ipAddress = it
                            sharedPreferences.edit().putString("esp32_ip", it.text).apply()
                        },
                        label = { Text("ESP32 IP Address") },
                        placeholder = { Text("e.g., 192.168.4.1") },
                        modifier = Modifier.fillMaxWidth(),
                        colors = OutlinedTextFieldDefaults.colors(
                            focusedBorderColor = MaterialTheme.colorScheme.primary,
                            unfocusedBorderColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
                        )
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        AnimatedButton(
                            text = "Bật đèn",
                            onClick = { sendCommand(ipAddress.text, "LIGHT_ON", scope) { status = it } }
                        )
                        AnimatedButton(
                            text = "Tắt đèn",
                            onClick = { sendCommand(ipAddress.text, "LIGHT_OFF", scope) { status = it } }
                        )
                    }
                    Spacer(modifier = Modifier.height(16.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        AnimatedButton(
                            text = "Mở cửa",
                            onClick = { sendCommand(ipAddress.text, "DOOR_OPEN", scope) { status = it } }
                        )
                        AnimatedButton(
                            text = "Đóng cửa",
                            onClick = { sendCommand(ipAddress.text, "DOOR_CLOSE", scope) { status = it } }
                        )
                    }
                }
            }
            Spacer(modifier = Modifier.height(16.dp))
            Text(text = status, style = MaterialTheme.typography.bodyMedium)
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ChangePasswordScreen(navController: NavController, sharedPreferences: SharedPreferences) {
    var oldPassword by remember { mutableStateOf(TextFieldValue("")) }
    var newPassword by remember { mutableStateOf(TextFieldValue("")) }
    var confirmNewPassword by remember { mutableStateOf(TextFieldValue("")) }
    var errorMessage by remember { mutableStateOf("") }
    val currentUser = sharedPreferences.getString("current_user", null)

    if (currentUser == null) {
        LaunchedEffect(Unit) {
            navController.navigate(Screen.Login.route) {
                popUpTo(Screen.ChangePassword.route) { inclusive = true }
            }
        }
        return
    }

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(text = "Change Password", style = MaterialTheme.typography.headlineMedium)
        Spacer(modifier = Modifier.height(16.dp))
        OutlinedTextField(
            value = oldPassword,
            onValueChange = { oldPassword = it },
            label = { Text("Old Password") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(8.dp))
        OutlinedTextField(
            value = newPassword,
            onValueChange = { newPassword = it },
            label = { Text("New Password") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(8.dp))
        OutlinedTextField(
            value = confirmNewPassword,
            onValueChange = { confirmNewPassword = it },
            label = { Text("Confirm New Password") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(16.dp))
        if (errorMessage.isNotEmpty()) {
            Text(text = errorMessage, color = MaterialTheme.colorScheme.error)
            Spacer(modifier = Modifier.height(8.dp))
        }
        Button(
            onClick = {
                val storedPassword = sharedPreferences.getString("password_$currentUser", null)
                when {
                    oldPassword.text.isEmpty() || newPassword.text.isEmpty() || confirmNewPassword.text.isEmpty() ->
                        errorMessage = "Please fill all fields"
                    storedPassword != oldPassword.text ->
                        errorMessage = "Old password is incorrect"
                    newPassword.text != confirmNewPassword.text ->
                        errorMessage = "New passwords do not match"
                    else -> {
                        sharedPreferences.edit().putString("password_$currentUser", newPassword.text).apply()
                        navController.navigate(Screen.Main.route) {
                            popUpTo(Screen.ChangePassword.route) { inclusive = true }
                        }
                    }
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Save")
        }
        Spacer(modifier = Modifier.height(8.dp))
        TextButton(onClick = { navController.navigate(Screen.Main.route) }) {
            Text("Cancel")
        }
    }
}

fun sendCommand(ip: String, command: String, scope: CoroutineScope, onResult: (String) -> Unit) {
    scope.launch {
        try {
            if (ip.isNotEmpty()) {
                onResult("Status: $command sent successfully (simulated)")
            } else {
                onResult("Status: Error - Please enter an IP address")
            }
        } catch (e: Exception) {
            onResult("Status: Error - ${e.message}")
        }
    }
}
