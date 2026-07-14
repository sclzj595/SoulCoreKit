# Security Policy

## Supported Versions

Currently, only the latest version of SoulCoreKit is supported with security updates.

| Version | Supported |
|---------|-----------|
| 1.0.x   | ✅ Yes    |

## Reporting a Vulnerability

If you discover a security vulnerability within SoulCoreKit, please follow these steps:

1. **Do not open a public issue** - Security vulnerabilities should be reported privately.

2. **Email the maintainers** at soulcorekit@gmail.com with the following information:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Any suggested fixes (optional)

3. **Wait for acknowledgment** - We will acknowledge your email within 48 hours.

4. **Work with us** - We will work with you to understand and fix the vulnerability.

## Security Guidelines

### Authentication
- Always use HTTPS for production environments
- Store tokens securely (never in plain text)
- Implement proper token expiration and refresh mechanisms

### Network Security
- Validate all incoming data
- Use TLS for all network communications
- Set appropriate timeout values to prevent DoS attacks
- Implement rate limiting for API endpoints

### Data Security
- Encrypt sensitive data at rest
- Sanitize all user inputs
- Use parameterized queries for database operations

### Dependencies
- Keep Qt and other dependencies up to date
- Monitor security advisories for Qt
- Regularly audit third-party libraries

## Security Features

SoulCoreKit includes the following security-related features:

- **AuthInterceptor**: Injects authentication tokens into requests
- **TokenManager**: Secure token storage and management
- **Permission System**: Role-based access control
- **Result<T>**: Type-safe error handling to prevent information leakage

## Responsible Disclosure

We believe in responsible disclosure. If you follow these guidelines, we will:

- Credit you in the release notes (if you wish)
- Work to fix the issue in a timely manner
- Keep you updated on progress

Thank you for helping keep SoulCoreKit secure!